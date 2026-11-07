#include <Servo.h>

Servo servos[6];

const byte pines[6] = {3, 5, 6, 9, 10, 11};

void setup()
{
    for (int i = 0; i < 6; i++)
    {
        servos[i].attach(pines[i]);
        servos[i].write(0);
    }
}

void loop()
{
    for (int i = 0; i < 6; i++)
    {
        servos[i].write(100);
        delay(1000);

        servos[i].write(0);
        delay(1000);
    }
}