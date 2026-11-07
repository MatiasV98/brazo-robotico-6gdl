# firmware

Programas de Arduino (`.ino`) del brazo robótico. Se abren con **Arduino IDE** y usan las librerías
`Servo`, `SoftwareSerial` y `EEPROM`, incluidas en el IDE. En clones del Nano, elegir
*ATmega328P (Old Bootloader)*.

| Carpeta | Descripción |
|---|---|
| `brazo_robusto/` | **Firmware actual.** Descripción en el [README principal](../README.md#firmware); comandos en [`docs/comandos-bluetooth.md`](../docs/comandos-bluetooth.md). |
| `version_anterior/` | Versión anterior del firmware, con la que se hicieron las primeras pruebas del brazo. Es la que describen el manual técnico y el archivo de limitaciones. |
| `prueba_limites/` | Herramienta para encontrar los límites mecánicos de cada articulación, moviéndola de a 1°. |
| `calibrador_servo_bt/` | Calibrador de un servo por Bluetooth, anterior a la PCB. |
| `prueba_todos_servos/` | Prueba automática de los 6 servos, sin Bluetooth. |
| `prueba_un_servo_bt/` | Prueba de un servo o de un conector de la placa por Bluetooth, con tres posiciones fijas. |

Antes de encender, leer [`docs/alimentacion-y-reinicios.md`](../docs/alimentacion-y-reinicios.md):
el Arduino se alimenta aparte de los servos.

## version_anterior

- **Pines:** Dedos D3, Muñeca D5, Mano D6, Codo D9, Brazo D10, Hombro D11. HC-05 en D13 (RX) y D12
  (TX), a 9600 baudios.
- **Movimiento progresivo:** una interrupción del Timer2 cada ~15 ms avanza 1° cada articulación hacia
  su destino, por lo que varias pueden moverse a la vez.
- **Arranque seguro:** al encender, todas las articulaciones van a 90° y los servos se habilitan de a uno.
- **Límites:** Brazo y Codo entre 60° y 120°; con el Brazo quieto en 90°, el Codo puede ir de 30° a 150°.
- **Comandos:** solo 0, 90 y 180 por articulación (`D0`, `MU90`, `C180`, …) y `X` para parar. Los
  mensajes de estado salen solo por el Monitor Serie (USB).

## prueba_limites

- `1` a `6`: selecciona la articulación (Dedos, Muñeca, Mano, Codo, Brazo, Hombro).
- `+` / `-`: mueve 1°. `0`, `9`, `8`: lleva la articulación a 0°, 90° o 180°.
- Al final del archivo están anotados los límites encontrados durante la calibración.

## calibrador_servo_bt

Se elige el pin en el código (`pinServo`) y el servo se mueve con `+`, `-`, `0`, `9` y `8`. Usa el
Bluetooth en los pines 2 y 3, no en los de la PCB.

## prueba_todos_servos

Mueve los 6 servos de a uno, entre 0° y 100°, en un ciclo continuo, para verificar servos y conectores.

## prueba_un_servo_bt

Mueve el servo de `PIN_SERVO` (por defecto D3, Dedos) a 0°, 90° o 180° con `1`, `2` y `3`. Usa los
pines de la PCB para el HC-05 (D13 RX, D12 TX).
