/*
  ============================================================================
   BRAZO ROBOTICO 6 SERVOS - TDII  (version robusta)
   Arduino Nano + HC-05 + 6 x MG996R con Servo.h
  ============================================================================
   Bluetooth: SoftwareSerial  D13 = RX (viene del TX del HC-05)
                              D12 = TX (va al RX del HC-05 por el divisor)
   Servos:    Dedos D3 | Muneca D5 | Mano D6 | Codo D9 | Brazo D10 | Hombro D11
   IMPORTANTE: no conectar servos en los conectores D0 y D1 (son el USB/Serial).

   Comandos (con o sin Enter, con o sin espacios, mayusculas o minusculas):
     D<ang>  MU<ang>  MA<ang>  C<ang>  B<ang>  H<ang>     ang = 0..180
     Ej: D0  D90  D180  MU45  c 120  b90
     X  -> PARADA inmediata (no hace falta Enter). Cancela tambien las secuencias.
     ?  -> estado actual de todos los servos
     S  -> SALUDO (secuencia predeterminada)
     I  -> posicion INICIAL segura (Codo, Brazo, Hombro, Mano, Muneca, Dedos)

   Movimiento de a un servo por vez (MAX_SERVOS_A_LA_VEZ):
     Si se piden varios movimientos, se hacen en el orden en que llegaron.
     Asi nunca arrancan varios MG996R juntos y se evitan los picos de corriente.

   Que cambio respecto de la version anterior:
   - Todos los textos van en memoria FLASH con F(): antes ocupaban mucha RAM
     (el Nano tiene solo 2 KB) y podian producir reinicios aleatorios.
   - No se usa la clase String (fragmenta la RAM). Se usa un buffer fijo.
   - No se bloquea esperando el '\n' (readStringUntil esperaba hasta 1 s).
   - La interrupcion del Timer2 solo marca el "tick" de 15 ms. El movimiento
     se hace en loop(). Asi la ISR dura microsegundos y no corrompe los bytes
     que recibe el SoftwareSerial.
   - Al arrancar informa POR QUE se reinicio (brown-out, encendido, reset...).
     Es la herramienta para diagnosticar el problema.
   - Guarda la ultima posicion en EEPROM: si se reinicia, vuelve a donde
     estaba en lugar de ir todo a 90.
   - Watchdog opcional (ver USAR_WATCHDOG).
  ============================================================================
*/

#include <Servo.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// ============================================================
// CONFIGURACION
// ============================================================
// Watchdog: reinicia el Arduino si el programa se cuelga.
// Dejarlo en 0 si el Nano tiene el "old bootloader": con ese bootloader un
// reset por watchdog lo deja reiniciandose en bucle. Ponerlo en 1 solo si se
// le grabo el bootloader Optiboot (en el IDE: placa Nano, procesador
// "ATmega328P", no "Old Bootloader").
#define USAR_WATCHDOG       0

// Respuestas cortas al celular ("OK C90", "ERR"...). Se mantienen cortas a
// proposito: mientras SoftwareSerial transmite, se bloquean las interrupciones
// y los servos pueden temblar.
#define RESPONDER_POR_BT    1

// Recordar la ultima posicion en EEPROM para que, tras un reinicio,
// los servos vuelvan a donde estaban y no todos a la posicion media.
#define GUARDAR_POSICIONES  1

// Cuantos servos se pueden mover al mismo tiempo. 1 = el mas seguro para la
// fuente. Con la alimentacion ya separada se puede probar 2.
#define MAX_SERVOS_A_LA_VEZ 1

const long VELOCIDAD_USB = 9600;
const long VELOCIDAD_BT  = 9600;

// ============================================================
// BLUETOOTH
// ============================================================
const byte PIN_BT_RX = 13;   // desde TX del HC-05
const byte PIN_BT_TX = 12;   // hacia RX del HC-05 (por el divisor de 3,3 V)
SoftwareSerial bluetooth(PIN_BT_RX, PIN_BT_TX);

// ============================================================
// ARTICULACIONES
// ============================================================
enum Articulacion : byte { DEDOS, MUNECA, MANO, CODO, BRAZO, HOMBRO, CANTIDAD };

Servo servos[CANTIDAD];

//                                    Dedos Muneca Mano Codo Brazo Hombro
const byte pinServo[CANTIDAD]     = {   3,    5,    6,   9,   10,   11 };
const byte anguloMinimo[CANTIDAD] = {   0,    0,    0,  60,   60,    0 };
const byte anguloMaximo[CANTIDAD] = { 180,  180,  180, 120,  120,  180 };

// Con el Brazo quieto en vertical, el Codo gana 30 grados por lado (30 - 150)
const byte LIBERTAD_EXTRA_CODO = 30;
const byte BRAZO_VERTICAL      = 90;

byte anguloActual[CANTIDAD];    // ultimo angulo enviado a cada servo
byte anguloDestino[CANTIDAD];   // angulo al que tiene que llegar

// Cola de movimientos: orden de llegada de cada pedido y servos activos
bool          enMovimiento[CANTIDAD];
unsigned long ordenPedido[CANTIDAD];
unsigned long contadorPedidos = 0;

// ============================================================
// SECUENCIAS PREDETERMINADAS (en FLASH)
// Cada paso: articulacion + angulo. Un paso empieza cuando terminaron
// todos los anteriores. PAUSA espera (angulo x 100 ms).
// Los angulos pasan igual por los limites de seguridad.
// ============================================================
struct Paso { byte articulacion; byte angulo; };
const byte PAUSA = 250;

const Paso SECUENCIA_SALUDO[] PROGMEM = {
    {DEDOS,   0},              // abre la mano
    {CODO,   90},              // brazo en posicion vertical
    {BRAZO,  90},
    {HOMBRO, 90},
    {MUNECA, 90},
    {MANO,   40},              // levanta la mano
    {MANO,  110}, {MANO,   40}, {MANO,  110}, {MANO,   40},     // "chau chau": mano arriba/abajo
    {MANO,  110}, {MANO,   40},
    {DEDOS, 120}, {DEDOS,   0}, {DEDOS, 120}, {DEDOS,   0},     // abre y cierra
    {PAUSA,   5},              // medio segundo quieto
    {MANO,   90},
    {DEDOS,  90},
};

const Paso SECUENCIA_INICIO[] PROGMEM = {
    {CODO, 90}, {BRAZO, 90}, {HOMBRO, 90}, {MANO, 90}, {MUNECA, 90}, {DEDOS, 90},
};

const Paso*   secuencia       = nullptr;
byte          largoSecuencia  = 0;
byte          pasoSecuencia   = 0;
unsigned long esperarHastaMs  = 0;
const unsigned long PAUSA_ENTRE_PASOS_MS = 200;

const int SIN_ESPERA = -1;
int brazoEnEspera = SIN_ESPERA; // movimiento del Brazo que espera al Codo

// La interrupcion solo avisa que pasaron ~15 ms
volatile bool tickMovimiento = false;

// ============================================================
// CAUSA DEL ULTIMO REINICIO
// Se lee el registro MCUSR antes que cualquier otra cosa del programa.
// ============================================================
uint8_t registroReset __attribute__((section(".noinit")));

void capturarCausaReset() __attribute__((naked, used, section(".init3")));
void capturarCausaReset()
{
    registroReset = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

// ============================================================
// EEPROM
// ============================================================
const int  EE_MARCA      = 0;     // byte de validacion
const int  EE_POSICIONES = 1;     // 6 bytes (1..6)
const int  EE_REINICIOS  = 10;    // contador de arranques (2 bytes)
const byte MARCA_VALIDA  = 0xB7;

bool          hayQueGuardar     = false;
unsigned long ultimoMovimientoMs = 0;

// ============================================================
// RECEPCION DE COMANDOS
// ============================================================
const byte    LARGO_MAXIMO        = 6;     // "MU180" + margen
const unsigned long FIN_POR_SILENCIO_MS = 80;  // si la app no manda Enter

char          bufferComando[LARGO_MAXIMO + 1];
byte          largoComando  = 0;
bool          comandoLargo  = false;
unsigned long ultimoByteMs  = 0;

// ============================================================
// TEXTOS (en FLASH)
// ============================================================
const __FlashStringHelper* nombre(byte a)
{
    switch (a)
    {
        case DEDOS:  return F("Dedos");
        case MUNECA: return F("Muneca");
        case MANO:   return F("Mano");
        case CODO:   return F("Codo");
        case BRAZO:  return F("Brazo");
        case HOMBRO: return F("Hombro");
    }
    return F("?");
}

const __FlashStringHelper* prefijo(byte a)
{
    switch (a)
    {
        case DEDOS:  return F("D");
        case MUNECA: return F("MU");
        case MANO:   return F("MA");
        case CODO:   return F("C");
        case BRAZO:  return F("B");
        case HOMBRO: return F("H");
    }
    return F("?");
}

int ramLibre()
{
    extern int __heap_start, *__brkval;
    int v;
    return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

// ============================================================
// TIMER2: interrupcion cada ~15 ms (16 MHz / 1024 / 234)
// ============================================================
ISR(TIMER2_COMPA_vect)
{
    tickMovimiento = true;
}

void iniciarTimerMovimiento()
{
    noInterrupts();
    TCCR2A = (1 << WGM21);                              // Modo CTC
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);   // Prescaler 1024
    TCNT2  = 0;
    OCR2A  = 233;
    TIMSK2 = (1 << OCIE2A);
    interrupts();
}

// ============================================================
// LIMITES Y SEGURIDAD
// ============================================================
bool codoFueraDeRangoNormal(byte angulo)
{
    return angulo < anguloMinimo[CODO] || angulo > anguloMaximo[CODO];
}

bool brazoQuietoEnVertical()
{
    return anguloActual[BRAZO]  == BRAZO_VERTICAL &&
           anguloDestino[BRAZO] == BRAZO_VERTICAL &&
           brazoEnEspera == SIN_ESPERA;
}

void calcularLimites(byte a, byte &inferior, byte &superior)
{
    inferior = anguloMinimo[a];
    superior = anguloMaximo[a];

    if (a == CODO && brazoQuietoEnVertical())
    {
        inferior -= LIBERTAD_EXTRA_CODO;
        superior += LIBERTAD_EXTRA_CODO;
    }
}

void responderBT(const __FlashStringHelper* texto)
{
#if RESPONDER_POR_BT
    bluetooth.println(texto);
#endif
}

// Fija el destino de una articulacion. El movimiento lo hace avanzarServos().
void moverArticulacion(byte a, int pedido)
{
    byte inferior, superior;
    calcularLimites(a, inferior, superior);
    int permitido = constrain(pedido, inferior, superior);

    if (a == BRAZO)
    {
        brazoEnEspera = SIN_ESPERA;   // una orden nueva reemplaza a la anterior

        // Si el Codo esta fuera de su rango normal, primero vuelve el Codo
        bool codoFuera = codoFueraDeRangoNormal(anguloActual[CODO]) ||
                         codoFueraDeRangoNormal(anguloDestino[CODO]);

        if (permitido != BRAZO_VERTICAL && codoFuera)
        {
            byte seguro = constrain(anguloDestino[CODO], anguloMinimo[CODO], anguloMaximo[CODO]);

            Serial.print(F("Seguridad: el Codo vuelve primero a "));
            Serial.print(seguro);
            Serial.println(F(" grados antes de mover el Brazo"));

            if (anguloDestino[CODO] != seguro) ordenPedido[CODO] = ++contadorPedidos;
            anguloDestino[CODO] = seguro;
            brazoEnEspera = permitido;
            hayQueGuardar = true;
            responderBT(F("OK CODO PRIMERO"));
            return;
        }
    }

    Serial.print(F("Accion: "));
    Serial.print(nombre(a));
    Serial.print(F(" de "));
    Serial.print(anguloActual[a]);
    Serial.print(F(" a "));
    Serial.print(permitido);
    Serial.print(F(" grados"));
    if (permitido != pedido)
    {
        Serial.print(F("  (pedido "));
        Serial.print(pedido);
        Serial.print(F(", limitado a ["));
        Serial.print(inferior);
        Serial.print(F(" - "));
        Serial.print(superior);
        Serial.print(F("])"));
    }
    Serial.println();

    // Si el servo ya se esta moviendo conserva su turno; si no, va al final de la cola
    if (!enMovimiento[a] && anguloDestino[a] != permitido)
        ordenPedido[a] = ++contadorPedidos;
    anguloDestino[a] = (byte)permitido;
    hayQueGuardar = true;

#if RESPONDER_POR_BT
    bluetooth.print(F("OK "));
    bluetooth.print(prefijo(a));
    bluetooth.println(permitido);
#endif
}

void cancelarSecuencia()
{
    if (secuencia == nullptr) return;
    secuencia = nullptr;
    Serial.println(F("Secuencia cancelada"));
    responderBT(F("SECUENCIA CANCELADA"));
}

void detenerTodo()
{
    cancelarSecuencia();
    for (byte a = 0; a < CANTIDAD; a++)
    {
        anguloDestino[a] = anguloActual[a];
        enMovimiento[a] = false;
    }
    brazoEnEspera = SIN_ESPERA;
    hayQueGuardar = true;

    Serial.println(F("PARADA: todos los servos quedan donde estan"));
    responderBT(F("PARADA"));
}

// ============================================================
// MOVIMIENTO (1 grado por tick de 15 ms)
// Solo se mueven hasta MAX_SERVOS_A_LA_VEZ servos. Los demas esperan su
// turno en el orden en que se pidieron.
// ============================================================
void elegirServosActivos()
{
    byte activos = 0;
    for (byte a = 0; a < CANTIDAD; a++)
    {
        if (enMovimiento[a] && anguloActual[a] == anguloDestino[a])
            enMovimiento[a] = false;
        if (enMovimiento[a]) activos++;
    }

    while (activos < MAX_SERVOS_A_LA_VEZ)
    {
        byte elegido = CANTIDAD;
        for (byte a = 0; a < CANTIDAD; a++)
        {
            if (enMovimiento[a] || anguloActual[a] == anguloDestino[a]) continue;
            if (elegido == CANTIDAD || ordenPedido[a] < ordenPedido[elegido])
                elegido = a;
        }
        if (elegido == CANTIDAD) break;   // nadie esperando
        enMovimiento[elegido] = true;
        activos++;
    }
}

void avanzarServos()
{
    elegirServosActivos();

    for (byte a = 0; a < CANTIDAD; a++)
    {
        if (!enMovimiento[a]) continue;

        if (anguloActual[a] < anguloDestino[a]) anguloActual[a]++;
        else                                    anguloActual[a]--;

        servos[a].write(anguloActual[a]);
        ultimoMovimientoMs = millis();

        if (anguloActual[a] == anguloDestino[a])
        {
            enMovimiento[a] = false;
            Serial.print(nombre(a));
            Serial.print(F(" llego a "));
            Serial.print(anguloActual[a]);
            Serial.println(F(" grados"));
            Serial.println(F("-----------------------------"));
        }
    }
}

// ============================================================
// SECUENCIAS
// ============================================================
bool algunoEnMovimiento();

void iniciarSecuencia(const Paso* pasos, byte largo, const __FlashStringHelper* nombreSecuencia)
{
    secuencia      = pasos;
    largoSecuencia = largo;
    pasoSecuencia  = 0;
    esperarHastaMs = millis();

    Serial.print(F("Secuencia: "));
    Serial.println(nombreSecuencia);
#if RESPONDER_POR_BT
    bluetooth.print(F("SECUENCIA "));
    bluetooth.println(nombreSecuencia);
#endif
}

void avanzarSecuencia()
{
    if (secuencia == nullptr) return;

    // Cada paso espera a que el anterior termine, mas una pausa corta
    if (algunoEnMovimiento())
    {
        esperarHastaMs = millis() + PAUSA_ENTRE_PASOS_MS;
        return;
    }
    if ((long)(millis() - esperarHastaMs) < 0) return;

    if (pasoSecuencia >= largoSecuencia)
    {
        secuencia = nullptr;
        Serial.println(F("Secuencia terminada"));
        responderBT(F("FIN SECUENCIA"));
        return;
    }

    Paso p;
    memcpy_P(&p, &secuencia[pasoSecuencia++], sizeof(Paso));

    if (p.articulacion == PAUSA)
        esperarHastaMs = millis() + p.angulo * 100UL;
    else
        moverArticulacion(p.articulacion, p.angulo);
}

void moverBrazoSiCodoListo()
{
    if (brazoEnEspera == SIN_ESPERA) return;
    if (anguloActual[CODO] != anguloDestino[CODO]) return;

    int angulo = brazoEnEspera;
    brazoEnEspera = SIN_ESPERA;

    Serial.println(F("Codo en rango seguro, ahora se mueve el Brazo"));
    moverArticulacion(BRAZO, angulo);
}

bool algunoEnMovimiento()
{
    for (byte a = 0; a < CANTIDAD; a++)
        if (anguloActual[a] != anguloDestino[a]) return true;
    return brazoEnEspera != SIN_ESPERA;
}

// ============================================================
// EEPROM: guardar cuando todo lleva 1,5 s quieto
// (EEPROM.update solo escribe si el valor cambio -> no la gasta)
// ============================================================
void guardarPosicionSiQuieto()
{
#if GUARDAR_POSICIONES
    if (!hayQueGuardar) return;
    if (algunoEnMovimiento()) return;
    if (millis() - ultimoMovimientoMs < 1500) return;

    for (byte a = 0; a < CANTIDAD; a++)
        EEPROM.update(EE_POSICIONES + a, anguloActual[a]);
    EEPROM.update(EE_MARCA, MARCA_VALIDA);
    hayQueGuardar = false;
#endif
}

void cargarPosicionInicial()
{
    // Posicion por defecto: el centro del rango de cada articulacion
    for (byte a = 0; a < CANTIDAD; a++)
        anguloDestino[a] = (anguloMinimo[a] + anguloMaximo[a]) / 2;

#if GUARDAR_POSICIONES
    if (EEPROM.read(EE_MARCA) != MARCA_VALIDA)
    {
        Serial.println(F("EEPROM sin posicion guardada: se usa la posicion media"));
        return;
    }

    byte guardado[CANTIDAD];
    for (byte a = 0; a < CANTIDAD; a++)
    {
        guardado[a] = EEPROM.read(EE_POSICIONES + a);
        if (guardado[a] > 180)
        {
            Serial.println(F("EEPROM con datos invalidos: se usa la posicion media"));
            return;
        }
    }

    // Se valida contra los limites; el Codo extendido solo si el Brazo quedo en 90
    for (byte a = 0; a < CANTIDAD; a++)
    {
        byte inferior = anguloMinimo[a];
        byte superior = anguloMaximo[a];
        if (a == CODO && guardado[BRAZO] == BRAZO_VERTICAL)
        {
            inferior -= LIBERTAD_EXTRA_CODO;
            superior += LIBERTAD_EXTRA_CODO;
        }
        anguloDestino[a] = constrain(guardado[a], inferior, superior);
    }
    Serial.println(F("Se recupera la ultima posicion guardada"));
#endif
}

// ============================================================
// DIAGNOSTICO
// ============================================================
void informarReinicio()
{
    unsigned int arranques;
    EEPROM.get(EE_REINICIOS, arranques);
    if (arranques == 0xFFFF) arranques = 0;
    arranques++;
    EEPROM.put(EE_REINICIOS, arranques);

    Serial.println();
    Serial.println(F("============================================"));
    Serial.print(F("Arranque numero "));
    Serial.println(arranques);
    Serial.print(F("Causa del reinicio: "));
    if (registroReset & (1 << PORF))  Serial.print(F("[ENCENDIDO] "));
    if (registroReset & (1 << EXTRF)) Serial.print(F("[RESET EXTERNO: boton o USB/monitor serie] "));
    if (registroReset & (1 << BORF))  Serial.print(F("[BROWN-OUT: cayo la tension de alimentacion] "));
    if (registroReset & (1 << WDRF))  Serial.print(F("[WATCHDOG: el programa se colgo] "));
    if (registroReset == 0)           Serial.print(F("[desconocida: el bootloader borro el registro]"));
    Serial.println();
    Serial.print(F("RAM libre: "));
    Serial.print(ramLibre());
    Serial.println(F(" bytes"));
    Serial.println(F("============================================"));

#if RESPONDER_POR_BT
    bluetooth.print(F("INICIO #"));
    bluetooth.print(arranques);
    if (registroReset & (1 << BORF))       bluetooth.println(F(" BROWNOUT"));
    else if (registroReset & (1 << WDRF))  bluetooth.println(F(" WATCHDOG"));
    else if (registroReset & (1 << EXTRF)) bluetooth.println(F(" RESET"));
    else if (registroReset & (1 << PORF))  bluetooth.println(F(" ENCENDIDO"));
    else                                   bluetooth.println();
#endif
}

void informarEstado()
{
    Serial.println(F("Estado actual:"));
    for (byte a = 0; a < CANTIDAD; a++)
    {
        Serial.print(F("  "));
        Serial.print(nombre(a));
        Serial.print(F(": "));
        Serial.print(anguloActual[a]);
        if (anguloActual[a] != anguloDestino[a])
        {
            Serial.print(F(" -> "));
            Serial.print(anguloDestino[a]);
        }
        Serial.println();
    }
    Serial.print(F("  RAM libre: "));
    Serial.println(ramLibre());

#if RESPONDER_POR_BT
    for (byte a = 0; a < CANTIDAD; a++)
    {
        bluetooth.print(prefijo(a));
        bluetooth.print(anguloActual[a]);
        bluetooth.print(' ');
    }
    bluetooth.println();
#endif
}

// ============================================================
// INTERPRETE DE COMANDOS
// Formato: prefijo (D, MU, MA, C, B, H) + numero de 1 a 3 cifras (0..180)
// ============================================================
void comandoInvalido()
{
    Serial.println(F("Comando no valido"));
    responderBT(F("ERR"));
}

void limpiarBuffer()
{
    largoComando = 0;
    comandoLargo = false;
}

void procesarComando()
{
    bufferComando[largoComando] = '\0';

    if (comandoLargo)
    {
        Serial.println(F("Comando demasiado largo"));
        responderBT(F("ERR LARGO"));
        limpiarBuffer();
        return;
    }
    if (largoComando == 0)
        return;

    Serial.print(F("Comando recibido: "));
    Serial.println(bufferComando);

    if (strcmp(bufferComando, "?") == 0)
    {
        informarEstado();
        limpiarBuffer();
        return;
    }

    if (strcmp(bufferComando, "S") == 0)
    {
        iniciarSecuencia(SECUENCIA_SALUDO,
                         sizeof(SECUENCIA_SALUDO) / sizeof(Paso), F("SALUDO"));
        limpiarBuffer();
        return;
    }

    if (strcmp(bufferComando, "I") == 0)
    {
        iniciarSecuencia(SECUENCIA_INICIO,
                         sizeof(SECUENCIA_INICIO) / sizeof(Paso), F("INICIO"));
        limpiarBuffer();
        return;
    }

    // Cualquier movimiento manual interrumpe la secuencia que se este ejecutando
    cancelarSecuencia();

    byte articulacion;
    byte i;

    if      (strncmp(bufferComando, "MU", 2) == 0) { articulacion = MUNECA; i = 2; }
    else if (strncmp(bufferComando, "MA", 2) == 0) { articulacion = MANO;   i = 2; }
    else
    {
        i = 1;
        switch (bufferComando[0])
        {
            case 'D': articulacion = DEDOS;  break;
            case 'C': articulacion = CODO;   break;
            case 'B': articulacion = BRAZO;  break;
            case 'H': articulacion = HOMBRO; break;
            default:  comandoInvalido(); limpiarBuffer(); return;
        }
    }

    // Numero: entre 1 y 3 cifras, todas digitos
    int angulo = 0;
    byte cifras = 0;
    while (bufferComando[i] != '\0')
    {
        char c = bufferComando[i++];
        if (c < '0' || c > '9' || ++cifras > 3)
        {
            comandoInvalido();
            limpiarBuffer();
            return;
        }
        angulo = angulo * 10 + (c - '0');
    }

    if (cifras == 0 || angulo > 180)
    {
        comandoInvalido();
        limpiarBuffer();
        return;
    }

    moverArticulacion(articulacion, angulo);
    limpiarBuffer();
}

// Lee lo que llega sin bloquear. Un comando termina con Enter (\n o \r)
// o cuando pasan 80 ms sin recibir nada (apps que no mandan Enter).
void leerBluetooth()
{
    while (bluetooth.available())
    {
        char c = bluetooth.read();
        ultimoByteMs = millis();

        if (c >= 'a' && c <= 'z') c -= 32;        // a mayusculas

        if (c == 'X')                              // parada: inmediata
        {
            limpiarBuffer();
            detenerTodo();
            continue;
        }

        if (c == '\n' || c == '\r')
        {
            procesarComando();
            continue;
        }

        if (c == ' ' || c == '\t' || c < 32 || c > 126)   // ignora basura
            continue;

        if (largoComando < LARGO_MAXIMO)
            bufferComando[largoComando++] = c;
        else
            comandoLargo = true;
    }

    if ((largoComando > 0 || comandoLargo) && millis() - ultimoByteMs > FIN_POR_SILENCIO_MS)
        procesarComando();

    if (bluetooth.overflow())
        Serial.println(F("Aviso: se perdieron datos del Bluetooth"));
}

// ============================================================
// SETUP / LOOP
// ============================================================
void setup()
{
    Serial.begin(VELOCIDAD_USB);
    bluetooth.begin(VELOCIDAD_BT);

    cargarPosicionInicial();

    // Arranque seguro: se fija el angulo ANTES de attach() para evitar el salto,
    // y se encienden de a uno para no sumar los picos de corriente.
    for (byte a = 0; a < CANTIDAD; a++)
    {
        anguloActual[a] = anguloDestino[a];
        servos[a].write(anguloActual[a]);
        servos[a].attach(pinServo[a]);
        delay(250);
    }

    informarReinicio();

    Serial.println(F("=== CONTROL DEL BRAZO ==="));
    Serial.println(F("Comandos: D MU MA C B H + angulo (0..180)"));
    Serial.println(F("  Ej: D0  D90  D180  MU45  C120  B90"));
    Serial.println(F("  Codo y Brazo: [60 - 120]"));
    Serial.println(F("  Codo con Brazo quieto en 90: [30 - 150]"));
    Serial.println(F("X = parada   ? = estado   S = saludo   I = posicion inicial"));
    Serial.print(F("Servos que se mueven a la vez: "));
    Serial.println(MAX_SERVOS_A_LA_VEZ);
    Serial.println(F("-----------------------------"));
    informarEstado();

    ultimoMovimientoMs = millis();
    iniciarTimerMovimiento();

#if USAR_WATCHDOG
    wdt_enable(WDTO_2S);
#endif
}

void loop()
{
#if USAR_WATCHDOG
    wdt_reset();
#endif

    leerBluetooth();

    if (tickMovimiento)
    {
        tickMovimiento = false;
        avanzarServos();
        moverBrazoSiCodoListo();
        avanzarSecuencia();
    }

    guardarPosicionSiQuieto();
}
