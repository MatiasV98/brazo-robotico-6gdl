#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial BT(13, 12);   // RX = D13 (desde TX del HC-05), TX = D12 (hacia RX del HC-05)

Servo servos[6];

const byte pines[6] = {3, 5, 6, 9, 10, 11};

int angulos[6] = {90, 90, 90, 90, 90, 90};

byte servoSeleccionado = 0;

void setup()
{
    Serial.begin(9600);
    BT.begin(9600);

    for (int i = 0; i < 6; i++)
    {
        servos[i].attach(pines[i]);
        servos[i].write(angulos[i]);
    }

    Serial.println("=== CONTROL DE BRAZO ROBOTICO ===");
    Serial.println();
    Serial.println("Seleccion de servo:");
    Serial.println("1 -> Dedos");
    Serial.println("2 -> Muneca");
    Serial.println("3 -> Mano");
    Serial.println("4 -> Codo");
    Serial.println("5 -> Brazo");
    Serial.println("6 -> Hombro");
    Serial.println();
    Serial.println("+ -> +1 grado");
    Serial.println("- -> -1 grado");
    Serial.println("0 -> 0 grados");
    Serial.println("9 -> 90 grados");
    Serial.println("8 -> 180 grados");
    Serial.println();
}

void loop()
{
    if (BT.available())
    {
        char dato = BT.read();

        switch (dato)
        {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
                servoSeleccionado = dato - '1';

                Serial.print("Servo seleccionado: ");
                Serial.println(servoSeleccionado + 1);
                return;

            case '+':
                if (angulos[servoSeleccionado] < 180)
                    angulos[servoSeleccionado]++;
                break;

            case '-':
                if (angulos[servoSeleccionado] > 0)
                    angulos[servoSeleccionado]--;
                break;

            case '0':
                angulos[servoSeleccionado] = 0;
                break;

            case '9':
                angulos[servoSeleccionado] = 90;
                break;

            case '8':
                angulos[servoSeleccionado] = 180;
                break;

            default:
                Serial.println("Comando no valido");
                return;
        }

        servos[servoSeleccionado].write(angulos[servoSeleccionado]);

        Serial.print("Servo ");
        Serial.print(servoSeleccionado + 1);
        Serial.print(" -> ");
        Serial.print(angulos[servoSeleccionado]);
        Serial.println(" grados");
    }
}

/*
                ACOTACIONES DEL BRAZO
===========================================================

EXTREMIDAD     |  ÁNGULO  |  POSICIÓN
----------------+----------+--------------------------------
DEDOS          |   0°     | Abiertos
               |  90°     | Semi cerrados
               | 180°     | Cerrados

MUÑECA         |   0°     | Giro máximo a la derecha
               |  90°     | Posición central
               | 180°     | Giro máximo a la izquierda

MANO           |   0°     | Elevada
               |  90°     | Inclinación media
               | 180°     | Baja completamente
               |          | (Limitado por el codo)

CODO           |   0°     | Extendido
               |  90°     | Flexión media
               | 180°     | Completamente flexionado

BRAZO          |   0°     | Abierto
               |  90°     | Posición media
               | 180°     | Cerrado

HOMBRO         |   0°     | Abierto
               |  90°     | Posición media
               | 180°     | Cerrado

                LÍMITES CALIBRADOS
===========================================================
ARRIBA - ABAJO
DEDOS   :  None - None
MUÑECA  :  None - None
MANO    :  None  - 160° (Si Codo->180°) - Inmovil (Si Brazo->180°/0°) - 180°(limite servo) (Si Brazo->180°/0° & Si Codo->180°)
CODO    :  None - None (Queda recto, asi que no hay problema)
BRAZO   :  Si Codo-> 90°, Brazo no puede superar los 60° y lo mismo para 120° - Si brazo: En 90° (No puede avanzar a 0°, Si Mano->0° y Si Codo->0°) - En 90° (No puede avanzar a 180°, Si Mano->180° y Si Codo->180°), osea no puede avanzar de ahi de 90 el brazo
HOMBRO  :  None  - None

===========================================================
*/
