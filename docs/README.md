# docs

Documentación del proyecto del brazo robótico.

## Contenido

| Archivo | Descripción |
|---|---|
| `alimentacion-y-reinicios.md` | Por qué hay que alimentar por separado el Arduino y los servos, conexión correcta, mejoras de hardware y otros problemas resueltos. |
| `comandos-bluetooth.md` | Comandos del firmware actual, rangos de cada articulación y conexión del HC-05. |
| `Manual_Tecnico_Brazo_Robotico.docx` | Manual técnico y de usuario: hardware, asignación de pines, guía de conexión, prueba del brazo y pautas para desarrollar un software propio. |
| `Limitaciones_y_Solucion_de_Problemas.txt` | Complemento del manual: limitaciones del hardware y del firmware, guía de solución de problemas, lista de verificación y pruebas básicas. |
| `Materiales_Brazo_Robotico.txt` | Lista de materiales con cantidades y links de compra en Mercado Libre. |

> El manual técnico y el archivo de limitaciones se escribieron para la versión anterior del firmware
> (`firmware/Codigo_Final-Brazo_Robotico`). Para el firmware actual (`firmware/brazo_robusto`) valen
> `comandos-bluetooth.md` y `alimentacion-y-reinicios.md`.

### Imágenes (`img/`)

| Archivo | Contenido |
|---|---|
| `brazo.jpg` | Foto del brazo robótico |
| `carcasa.jpg` | Diseño de la carcasa de protección |
| `esquematico.png` | Esquemático del circuito |
| `pcb_3d.jpg` | Vista 3D de la PCB |
| `pcb_cobre.jpg` | Cobre de la PCB, cara inferior |
| `panel_web.png` | Panel web de control |

## Por dónde empezar

1. **Antes de encender:** `alimentacion-y-reinicios.md`.
2. **Para armar y probar el brazo:** `Manual_Tecnico_Brazo_Robotico.docx`.
3. **Para controlarlo:** `comandos-bluetooth.md` y el panel web (`web/control_brazo.html`).
4. **Si algo no funciona:** `Limitaciones_y_Solucion_de_Problemas.txt`.
