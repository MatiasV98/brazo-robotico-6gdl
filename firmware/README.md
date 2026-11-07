# firmware

Programas de Arduino (`.ino`) del brazo robótico. Se abren con **Arduino IDE** y usan las librerías `Servo`, `SoftwareSerial` y `EEPROM`, incluidas en el IDE. En clones del Nano, elegir *ATmega328P (Old Bootloader)*.

## Contenido

| Carpeta | Programa | Descripción |
|---|---|---|
| `brazo_robusto/` | `brazo_robusto.ino` | **Firmware actual.** Control de las 6 articulaciones por Bluetooth, un servo a la vez, límites de seguridad, secuencias, memoria EEPROM y diagnóstico de reinicios. Se usa con el panel web (`web/control_brazo.html`). |
| `Codigo_Final-Brazo_Robotico/` | `Codigo_Final-Brazo_Robotico.ino` | Versión anterior del firmware. |
| `Prueba_De_Limites/` | `Prueba_De_Limites.ino` | Herramienta para encontrar los límites de cada articulación moviéndola de a 1 grado. |
| `Movimiento_Bluetooth_Todos_Servos/` | `Movimientos_Con_Bluetooth.ino` | Calibrador de un servo por Bluetooth (prueba anterior). |
| `Movimiento_Automaticos_Todos_Servos/` | `Movimiento_Automaticos_Todos_Servos.ino` | Prueba automática de los 6 servos, sin Bluetooth. |
| `Movimiento_Automatico_Un_Servo/` | `Movimiento_Un_Servo.ino` | Prueba de un servo por Bluetooth con tres posiciones fijas. |

---

## brazo_robusto

Firmware actual del brazo.

- **Movimiento progresivo** de 1° cada 15 ms. La interrupción del Timer2 solo marca el tiempo y el movimiento se hace en `loop()`, así no corrompe los bytes que recibe el SoftwareSerial.
- **Un servo a la vez:** los pedidos se ejecutan en el orden en que llegan, para evitar picos de corriente.
- **Seguridad entre Codo y Brazo:** si el Codo está fuera de 60–120°, primero vuelve a ese rango y recién después se mueve el Brazo.
- **Secuencias:** Saludo (`S`) y Posición inicial (`I`). La parada o cualquier comando manual las cancela.
- **Diagnóstico de reinicios:** al arrancar informa la causa (brown-out, encendido, reset o watchdog) por USB y por Bluetooth.
- **Memoria EEPROM:** guarda la última posición; después de un reinicio vuelve a donde estaba.
- **Recepción sin bloqueo:** el comando termina con Enter o tras 80 ms sin datos; `X` frena al instante.

Comandos, rangos y conexión del HC-05 en [`docs/comandos-bluetooth.md`](../docs/comandos-bluetooth.md). Antes de encender, leer [`docs/alimentacion-y-reinicios.md`](../docs/alimentacion-y-reinicios.md): el Arduino se alimenta aparte de los servos.

## Codigo_Final-Brazo_Robotico

Versión anterior del firmware, con la que se hicieron las primeras pruebas del brazo. La reemplaza `brazo_robusto`.

- **Pines:** Dedos D3, Muñeca D5, Mano D6, Codo D9, Brazo D10, Hombro D11. HC-05 en D13 (RX) y D12 (TX), a 9600 baudios.
- **Movimiento progresivo:** una interrupción del Timer2 cada ~15 ms avanza 1° cada articulación hacia su destino, por lo que varias pueden moverse a la vez sin bloquear la recepción de comandos.
- **Arranque seguro:** al encender, todas las articulaciones van a 90° y los servos se habilitan de a uno.
- **Límites de seguridad:** Brazo y Codo entre 60° y 120°. Con el Brazo quieto en 90° (vertical), el Codo puede ir de 30° a 150°. Si se mueve el Brazo con el Codo fuera de rango, primero vuelve el Codo.
- **Mensajes:** informa por el Monitor Serie (USB, 9600 baudios) cada comando recibido, la acción realizada y cuándo llega cada articulación.

Comandos (texto terminado en salto de línea):

| Articulación | Comandos |
|---|---|
| Dedos | `D0` `D90` `D180` |
| Muñeca | `MU0` `MU90` `MU180` |
| Mano | `MA0` `MA90` `MA180` |
| Codo | `C0` `C90` `C180` |
| Brazo | `B0` `B90` `B180` |
| Hombro | `H0` `H90` `H180` |
| Parada | `X` |

El detalle completo está en `docs/Manual_Tecnico_Brazo_Robotico.docx`.

## Prueba_De_Limites

Permite mover cada articulación de forma manual por Bluetooth para encontrar sus límites mecánicos.

- `1` a `6`: selecciona la articulación (Dedos, Muñeca, Mano, Codo, Brazo, Hombro).
- `+` / `-`: mueve 1 grado.
- `0`, `9`, `8`: lleva la articulación a 0°, 90° o 180°.

Al final del archivo están anotadas las posiciones de cada articulación y los límites encontrados durante la calibración.

Usa los mismos pines que la PCB definitiva: HC-05 en D13 (RX) y D12 (TX).

## Movimiento_Bluetooth_Todos_Servos

Calibrador de **un solo servo** por Bluetooth: se elige el pin en el código (`pinServo`) y se mueve con `+`, `-`, `0`, `9` y `8`. Incluye en comentarios la tabla de posiciones de cada articulación.

> Es una versión anterior a la PCB: usa el Bluetooth en los pines 2 y 3.

## Movimiento_Automaticos_Todos_Servos

Mueve los 6 servos de a uno, entre 0° y 100°, en un ciclo continuo. Sirve para verificar que todos los servos y conectores funcionen. No usa Bluetooth.

## Movimiento_Automatico_Un_Servo

Prueba de **un servo** por Bluetooth, pensada para verificar un servo o un conector de la placa por separado.

- Por defecto mueve el servo del pin D3 (Dedos); para probar otro, cambiar `PIN_SERVO` en el código.
- `1`, `2` y `3`: llevan el servo a 0°, 90° y 180°.
- Usa los pines de la PCB definitiva para el HC-05: D13 (RX) y D12 (TX).
