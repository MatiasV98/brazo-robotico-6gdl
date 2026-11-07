#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial bluetooth(13, 12);   // D13 = RX (desde TX del HC-05), D12 = TX (hacia RX del HC-05)

// ============================================================
// ARTICULACIONES
// ============================================================
const byte DEDOS  = 0;
const byte MUNECA = 1;
const byte MANO   = 2;
const byte CODO   = 3;
const byte BRAZO  = 4;
const byte HOMBRO = 5;

const byte CANTIDAD_ARTICULACIONES = 6;

Servo servos[CANTIDAD_ARTICULACIONES];

//                                                 Dedos  Muneca  Mano  Codo  Brazo  Hombro
const byte pinServo[CANTIDAD_ARTICULACIONES]      = {  3,      5,    6,    9,   10,     11};
const char* nombreArticulacion[CANTIDAD_ARTICULACIONES] = {"Dedos", "Muneca", "Mano", "Codo", "Brazo", "Hombro"};

// ============================================================
// LIMITES DE SEGURIDAD (en grados)
// Codo y Brazo se limitan para que el brazo no golpee la mesa.
// ============================================================
//                                                 Dedos  Muneca  Mano  Codo  Brazo  Hombro
const byte anguloMinimo[CANTIDAD_ARTICULACIONES] = {   0,      0,    0,   60,    60,      0};
const byte anguloMaximo[CANTIDAD_ARTICULACIONES] = { 180,    180,  180,  120,   120,    180};

// Con el Brazo vertical, el Codo gana 30 grados por lado (30 - 150)
const byte LIBERTAD_EXTRA_CODO = 30;
const byte BRAZO_VERTICAL      = 90;

// ============================================================
// ESTADO DE LOS SERVOS (compartido con la interrupcion)
// ============================================================
volatile byte anguloActual[CANTIDAD_ARTICULACIONES];
volatile byte anguloDestino[CANTIDAD_ARTICULACIONES];
volatile byte servosQueLlegaron = 0;   // Bit N = 1 cuando el servo N llego a destino

// Movimiento del Brazo que espera a que el Codo vuelva a su rango normal
const int SIN_ESPERA = -1;
int anguloBrazoEnEspera = SIN_ESPERA;

// ============================================================
// INTERRUPCION DEL TIMER2 (cada ~15 ms)
// Avanza 1 grado cada servo que todavia no llego a su destino.
// ============================================================
ISR(TIMER2_COMPA_vect)
{
    for (byte articulacion = 0; articulacion < CANTIDAD_ARTICULACIONES; articulacion++)
    {
        if (anguloActual[articulacion] == anguloDestino[articulacion])
            continue;

        if (anguloActual[articulacion] < anguloDestino[articulacion])
            anguloActual[articulacion]++;
        else
            anguloActual[articulacion]--;

        servos[articulacion].write(anguloActual[articulacion]);

        if (anguloActual[articulacion] == anguloDestino[articulacion])
            servosQueLlegaron |= (1 << articulacion);
    }
}

// Timer2 en modo CTC: 16 MHz / 1024 / (233 + 1) = ~15 ms
void iniciarTimerMovimiento()
{
    noInterrupts();

    TCCR2A = (1 << WGM21);                              // Modo CTC
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);   // Prescaler 1024
    TCNT2  = 0;
    OCR2A  = 233;
    TIMSK2 = (1 << OCIE2A);                              // Habilitar interrupcion

    interrupts();
}

bool brazoQuietoEnVertical()
{
    return anguloActual[BRAZO] == BRAZO_VERTICAL && anguloDestino[BRAZO] == BRAZO_VERTICAL;
}

void calcularLimites(byte articulacion, int &limiteInferior, int &limiteSuperior)
{
    limiteInferior = anguloMinimo[articulacion];
    limiteSuperior = anguloMaximo[articulacion];

    if (articulacion == CODO && brazoQuietoEnVertical())
    {
        limiteInferior -= LIBERTAD_EXTRA_CODO;
        limiteSuperior += LIBERTAD_EXTRA_CODO;
    }
}

// Fija el destino de una articulacion; el movimiento lo hace la interrupcion
void moverArticulacion(byte articulacion, int anguloPedido)
{
    int limiteInferior, limiteSuperior;
    calcularLimites(articulacion, limiteInferior, limiteSuperior);

    int anguloPermitido = anguloPedido;
    if (anguloPermitido < limiteInferior) anguloPermitido = limiteInferior;
    if (anguloPermitido > limiteSuperior) anguloPermitido = limiteSuperior;

    // Si el Codo esta fuera de su rango normal, primero vuelve el Codo y despues se mueve el Brazo
    if (articulacion == BRAZO && anguloPermitido != BRAZO_VERTICAL)
    {
        byte codoMinimoNormal = anguloMinimo[CODO];
        byte codoMaximoNormal = anguloMaximo[CODO];
        byte destinoCodo      = anguloDestino[CODO];

        bool codoFueraDeRango =
            destinoCodo < codoMinimoNormal || destinoCodo > codoMaximoNormal ||
            anguloActual[CODO] < codoMinimoNormal || anguloActual[CODO] > codoMaximoNormal;

        if (codoFueraDeRango)
        {
            byte anguloSeguroCodo = destinoCodo;
            if (destinoCodo < codoMinimoNormal) anguloSeguroCodo = codoMinimoNormal;
            if (destinoCodo > codoMaximoNormal) anguloSeguroCodo = codoMaximoNormal;

            Serial.print("Seguridad: el Codo vuelve primero a ");
            Serial.print(anguloSeguroCodo);
            Serial.println(" grados antes de mover el Brazo");

            anguloDestino[CODO] = anguloSeguroCodo;
            anguloBrazoEnEspera = anguloPermitido;
            return;
        }
    }

    Serial.print("Accion: ");
    Serial.print(nombreArticulacion[articulacion]);
    Serial.print(" de ");
    Serial.print(anguloActual[articulacion]);
    Serial.print(" a ");
    Serial.print(anguloPermitido);
    Serial.print(" grados");

    if (anguloPermitido != anguloPedido)
    {
        Serial.print("  (pedido ");
        Serial.print(anguloPedido);
        Serial.print(", limitado a [");
        Serial.print(limiteInferior);
        Serial.print(" - ");
        Serial.print(limiteSuperior);
        Serial.print("])");
    }
    Serial.println();

    anguloDestino[articulacion] = anguloPermitido;
}

void moverBrazoSiCodoListo()
{
    if (anguloBrazoEnEspera == SIN_ESPERA)
        return;

    if (anguloActual[CODO] != anguloDestino[CODO])
        return;

    int anguloBrazo = anguloBrazoEnEspera;
    anguloBrazoEnEspera = SIN_ESPERA;

    Serial.println("Codo en rango seguro, ahora se mueve el Brazo");
    moverArticulacion(BRAZO, anguloBrazo);
}

void detenerTodo()
{
    noInterrupts();
    for (byte articulacion = 0; articulacion < CANTIDAD_ARTICULACIONES; articulacion++)
        anguloDestino[articulacion] = anguloActual[articulacion];
    interrupts();

    anguloBrazoEnEspera = SIN_ESPERA;

    Serial.println("PARADA: todos los servos quedan donde estan");
}

void informarLlegadas()
{
    if (servosQueLlegaron == 0)
        return;

    noInterrupts();
    byte llegaron = servosQueLlegaron;
    servosQueLlegaron = 0;
    interrupts();

    for (byte articulacion = 0; articulacion < CANTIDAD_ARTICULACIONES; articulacion++)
    {
        if (llegaron & (1 << articulacion))
        {
            Serial.print(nombreArticulacion[articulacion]);
            Serial.print(" llego a ");
            Serial.print(anguloActual[articulacion]);
            Serial.println(" grados");
            Serial.println("-----------------------------");
        }
    }
}

void setup()
{
    Serial.begin(9600);
    bluetooth.begin(9600);

    // Arranque seguro: cada servo va a 90 grados antes de attach() para evitar
    // el salto inicial, y se encienden de a uno para no sumar picos de corriente
    for (byte articulacion = 0; articulacion < CANTIDAD_ARTICULACIONES; articulacion++)
    {
        byte anguloInicial = (anguloMinimo[articulacion] + anguloMaximo[articulacion]) / 2;
        anguloActual[articulacion]  = anguloInicial;
        anguloDestino[articulacion] = anguloInicial;

        servos[articulacion].write(anguloInicial);
        servos[articulacion].attach(pinServo[articulacion]);
        delay(300);
    }

    iniciarTimerMovimiento();

    Serial.println("=== CONTROL DEL BRAZO (con interrupciones) ===");
    Serial.println("Comandos (entre corchetes, donde queda realmente):");
    Serial.println("Dedos:  D0    D90    D180");
    Serial.println("Muneca: MU0   MU90   MU180");
    Serial.println("Mano:   MA0   MA90   MA180");
    Serial.println("Codo:   C0    C90    C180    [60 / 90 / 120]");
    Serial.println("        con Brazo en 90:     [30 / 90 / 150]");
    Serial.println("Brazo:  B0    B90    B180    [60 / 90 / 120]");
    Serial.println("Hombro: H0    H90    H180");
    Serial.println("X = detener todo");
    Serial.println("-----------------------------");
    Serial.println("Posicion inicial: todos en 90");
    Serial.println("-----------------------------");
}

void loop()
{
    informarLlegadas();
    moverBrazoSiCodoListo();

    if (bluetooth.available())
    {
        String comando = bluetooth.readStringUntil('\n');
        comando.trim();
        comando.toUpperCase();
        comando.replace(" ", "");

        if (comando.length() == 0)
            return;

        if (comando.length() > 5)
        {
            Serial.println("Comando demasiado largo");
            return;
        }

        Serial.print("Comando recibido: ");
        Serial.println(comando);

        if (comando == "X")             detenerTodo();

        else if (comando == "D0")       moverArticulacion(DEDOS, 0);
        else if (comando == "D90")      moverArticulacion(DEDOS, 90);
        else if (comando == "D180")     moverArticulacion(DEDOS, 180);

        else if (comando == "MU0")      moverArticulacion(MUNECA, 0);
        else if (comando == "MU90")     moverArticulacion(MUNECA, 90);
        else if (comando == "MU180")    moverArticulacion(MUNECA, 180);

        else if (comando == "MA0")      moverArticulacion(MANO, 0);
        else if (comando == "MA90")     moverArticulacion(MANO, 90);
        else if (comando == "MA180")    moverArticulacion(MANO, 180);

        else if (comando == "C0")       moverArticulacion(CODO, 0);
        else if (comando == "C90")      moverArticulacion(CODO, 90);
        else if (comando == "C180")     moverArticulacion(CODO, 180);

        else if (comando == "B0")       moverArticulacion(BRAZO, 0);
        else if (comando == "B90")      moverArticulacion(BRAZO, 90);
        else if (comando == "B180")     moverArticulacion(BRAZO, 180);

        else if (comando == "H0")       moverArticulacion(HOMBRO, 0);
        else if (comando == "H90")      moverArticulacion(HOMBRO, 90);
        else if (comando == "H180")     moverArticulacion(HOMBRO, 180);

        else
            Serial.println("Comando no valido");
    }
}