# hardware

Diseño de la placa de control del brazo robótico (PCB), realizado en **KiCad 10**.

## Contenido

### `Placa Definitiva-Proyecto Brazo Robotico/` — Proyecto de KiCad (archivos fuente)

| Archivo | Descripción |
|---|---|
| `.kicad_pro` | Proyecto |
| `.kicad_sch` | Esquemático |
| `.kicad_pcb` | Diseño de la PCB |
| `.kicad_prl` | Preferencias locales del proyecto |

### `Archivos Gerber/` — Archivos de fabricación (Gerber y taladrado)

| Archivo | Descripción |
|---|---|
| `-B_Cu.gbr` | Cobre, cara inferior |
| `-B_Mask.gbr` | Máscara antisoldante, cara inferior |
| `-F_Mask.gbr` | Máscara antisoldante, cara superior |
| `-B_Paste.gbr` | Pasta, cara inferior |
| `-F_Paste.gbr` | Pasta, cara superior |
| `-B_Silkscreen.gbr` | Serigrafía, cara inferior |
| `-F_Silkscreen.gbr` | Serigrafía, cara superior |
| `-Edge_Cuts.gbr` | Contorno de la placa |
| `-PTH.drl` | Taladrado de agujeros metalizados |
| `-NPTH.drl` | Taladrado de agujeros no metalizados |
| `-job.gbrjob` | Datos del trabajo de fabricación |

Todos los archivos comienzan con `Placa Definitiva-Proyecto Brazo Robotico`.

Para abrir el diseño: abrir `Placa Definitiva-Proyecto Brazo Robotico.kicad_pro` con KiCad 10 o superior.

## La placa

| Característica | Valor |
|---|---|
| Dimensiones | 50 × 50 mm |
| Espesor | 1,6 mm |
| Capas | 2 (todo el ruteo en la cara inferior) |
| Componentes | Through-hole (THT) |

Componentes que aloja:

- **Arduino Nano** (A1), montado en zócalo de tiras hembra 1x15.
- **12 conectores para servos** (tiras hembra 1x3), unidos a los pines D0 a D11. El brazo usa D3, D5, D6, D9, D10 y D11.
- **Conector del módulo Bluetooth HC-05** (tira hembra 1x4: RX, TX, GND, 5V), conectado a D12 y D13.
- **Divisor de tensión** para el RX del HC-05: R1, R2 y R3 de 1 kΩ.
- **Conector de entradas analógicas** A0 a A7 (tira hembra 1x8).

| Articulación | Pin |
|---|---|
| Dedos | D3 |
| Muñeca | D5 |
| Mano | D6 |
| Codo | D9 |
| Brazo | D10 |
| Hombro | D11 |

La guía de conexión completa está en el manual técnico (`docs/Manual_Tecnico_Brazo_Robotico.docx`) y la lista de materiales en `docs/Materiales_Brazo_Robotico.txt`.

## Observaciones

- Por un error en el diseño de la placa, el **Arduino Nano queda montado en posición invertida** respecto de la orientación prevista. Al armar, respetar la orientación indicada en la Figura 1 del manual. Queda pendiente corregirlo en una próxima revisión.
- Los conectores D0 y D1 comparten pines con el puerto USB del Nano: si se conecta algo en ellos, puede interferir al cargar el programa o al usar el Monitor Serie.
