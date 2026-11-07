# Comandos por Bluetooth

Firmware `firmware/brazo_robusto`. Módulo HC-05 a 9600 baudios.

| Comando | Acción |
|---|---|
| `D<ang>` `MU<ang>` `MA<ang>` `C<ang>` `B<ang>` `H<ang>` | Mueve la articulación a un ángulo entre 0 y 180 (por ejemplo `C90`, `MU45`). Acepta minúsculas y espacios. |
| `X` | Parada inmediata (no necesita Enter). También cancela las secuencias. |
| `?` | Estado actual. Responde, por ejemplo, `D90 MU90 MA90 C90 B90 H90`. |
| `S` | Secuencia de saludo. |
| `I` | Posición inicial segura. |

Un comando termina con Enter o después de 80 ms sin recibir datos.

Respuestas cortas por Bluetooth: `OK C90`, `ERR`, `PARADA`, `SECUENCIA SALUDO`, `FIN SECUENCIA`,
`INICIO #n BROWNOUT`, entre otras. Se mantienen cortas a propósito: mientras SoftwareSerial transmite,
las interrupciones quedan bloqueadas y los servos pueden temblar.

## Articulaciones y rangos

| Articulación | Pin | Prefijo | Rango permitido |
|---|---|---|---|
| Dedos | D3 | `D` | 0–180 (0 = abiertos) |
| Muñeca | D5 | `MU` | 0–180 |
| Mano | D6 | `MA` | 0–180 (0 = arriba) |
| Codo | D9 | `C` | 60–120 (30–150 solo con el Brazo quieto en 90) |
| Brazo | D10 | `B` | 60–120 |
| Hombro | D11 | `H` | 0–180 |

Si el Codo está fuera de 60–120 y se pide mover el Brazo, primero vuelve el Codo a ese rango y recién
después se mueve el Brazo.

## Conexión del HC-05

| Señal | Pin |
|---|---|
| TX del HC-05 → Arduino | D13 (RX del SoftwareSerial) |
| Arduino → RX del HC-05 | D12, a través de un divisor a 3,3 V (R1 ‖ R2 = 500 Ω arriba, R3 = 1 kΩ a GND) |
