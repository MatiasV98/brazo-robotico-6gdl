#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial BT(2, 3);   // RX, TX
Servo servo;

const byte pinServo = 9;   // Cambiá este pin según el servo que pruebes

int angulo = 90;           // Posición inicial

void setup()
{
    Serial.begin(9600);
    BT.begin(9600);

    servo.attach(pinServo);
    servo.write(angulo);

    Serial.println("=== CALIBRADOR DE SERVO ===");
    Serial.println("Comandos:");
    Serial.println("+  -> +1 grado");
    Serial.println("-  -> -1 grado");
    Serial.println("0  -> 0 grados");
    Serial.println("9  -> 90 grados");
    Serial.println("8  -> 180 grados");
    Serial.println();

    Serial.print("Angulo inicial: ");
    Serial.println(angulo);
}

void loop()
{
    if (BT.available())
    {
        char dato = BT.read();

        switch (dato)
        {
            case '+':
                if (angulo < 180)
                    angulo++;
                break;

            case '-':
                if (angulo > 0)
                    angulo--;
                break;

            case '0':
                angulo = 0;
                break;

            case '9':
                angulo = 90;
                break;

            case '8':
                angulo = 180;
                break;

            default:
                Serial.println("Comando no valido");
                return;
        }

        servo.write(angulo);

        Serial.print("Angulo actual: ");
        Serial.print(angulo);
        Serial.println(" grados");
    }
}


/*
===========================================================
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

===========================================================
*/
/*
===========================================================
                    LÍMITES CALIBRADOS
===========================================================
            ARRIBA - ABAJO
 DEDOS   :  None - None  
 MUÑECA  :  None - None
 MANO    :  None  - 160° (Codo->180°) - Inmovil (Brazo->180°/0°) - 180°(limite servo) (Brazo->180°/0° & Codo->180°)
 CODO    :  None - None
 BRAZO   :  90° (Mano->90°)  - 160° (Cerrado)
 HOMBRO  :  None  - None 

===========================================================
*/