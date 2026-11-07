// Prueba de un servo por Bluetooth.
// Comandos: '1' = 0 grados, '2' = 90 grados, '3' = 180 grados.

#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial BT(13, 12);   // RX = D13 (desde TX del HC-05), TX = D12 (hacia RX del HC-05)

const byte PIN_SERVO = 3;    // D3 = Dedos. Cambiar para probar otro conector.

Servo servo;

void mover(int angulo)
{
  servo.write(angulo);
  Serial.print("Moviendo a ");
  Serial.println(angulo);
}

void setup()
{
  Serial.begin(9600);
  BT.begin(9600);

  servo.write(90);
  servo.attach(PIN_SERVO);

  delay(1000);

  mover(90);

  Serial.println("Listo");
}

void loop()
{
  if(BT.available())
  {
    char c = BT.read();

    Serial.print("Recibido: ");
    Serial.println(c);

    if(c=='1') mover(0);
    if(c=='2') mover(90);
    if(c=='3') mover(180);
  }
}
