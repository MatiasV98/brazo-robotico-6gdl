# Alimentación y reinicios

## Alimentar por separado el Arduino y los servos

**El Arduino no debe tomar alimentación del mismo riel que los servos.**

En la PCB original, el riel de los servos también iba al pin VIN del Nano. Cuando los MG996R hacen
fuerza (al arrancar, al cargar peso o al trabarse), cada uno pide picos de 2 a 3 A. Esos picos duran
milisegundos y no se ven con el tester, pero hacen caer la tensión: el Nano se reinicia por brown-out,
el brazo "se cae" y después vuelve a la posición inicial.

**Se comprobó en la práctica:** con el Arduino alimentado aparte, los reinicios desaparecieron por
completo.

## Conexión correcta

- **Servos:** fuente externa, con fusible, conectada a la bornera. Se recomienda unos 6 V y 10 a 15 A.
- **Arduino:** alimentación independiente, por USB, con 7 a 12 V en VIN o con un regulador de 5 V aparte
  en el pin 5V. **Nunca desde el riel de los servos.**
- **GND común** entre la fuente de los servos y el Arduino. Es obligatorio: sin masa común, las señales
  de control no tienen referencia.

## Mejoras de hardware recomendadas

- Capacitor electrolítico de 1000 a 2200 µF / 16 V en la bornera de los servos.
- 220 a 470 µF cerca de los conectores de los servos.
- 100 nF + 10 µF en el 5V del Nano y en el VCC del HC-05.
- Pistas de potencia anchas (3 a 5 mm) o reforzadas con cable.
- Masa en estrella: el retorno de los servos va directo a la bornera, sin pasar por el GND del Nano.

## Otros problemas resueltos

- **Error `not in sync: resp=0x00` al cargar el programa:** elegir *Procesador → ATmega328P (Old
  Bootloader)* en el IDE, dejar libres D0 y D1 (o cargar con el Nano fuera de la PCB) y cerrar el
  Monitor Serie.
- **El Monitor Serie reinicia el Nano cada vez que se abre.** Es el comportamiento normal del Nano, no
  una falla.
- **No conectar servos en los conectores D0 y D1 de la PCB:** son el puerto serie del USB y tienen que
  quedar libres para cargar el programa.
