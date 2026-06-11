# Menú en Pantalla

SVITRIX proporciona un **menú en pantalla** directamente en tu reloj.
Mantén presionado el botón del medio por 2 segundos para acceder al menú.
Navega a través de los elementos con los botones izquierdo y derecho y elige el submenú presionando el botón del medio.
Mantén presionado el botón del medio por 2 segundos para salir del menú actual y guardar tu configuración.

::: warning
Puedes fácilmente encender o apagar tu matriz SVITRIX simplemente presionando dos veces el botón del medio si no estás en el Menú.
:::

El menú está en **español** y consta de 14 elementos.

| Elemento del Menú | Descripción |
| --- | --- |
| `BRILLO` | Ajusta el brillo de la pantalla (0–100 %) o `AUTO`. Cambia entre automático y manual con el botón del medio. |
| `COLOR` | Selecciona uno de 15 colores para el texto (se muestra el valor hexadecimal). |
| `ROTACION` | **Submenú** con los ajustes de rotación de apps (ver abajo). |
| `HORA` | Selecciona el formato de hora. |
| `FECHA` | Selecciona el formato de fecha. |
| `INI-SEM` | Selecciona el inicio de semana (`LUN`/`DOM`). |
| `TEMP` | Selecciona el sistema de temperatura (°C o °F). |
| `APPS` | Habilita o deshabilita las apps internas. |
| `NOCHE` | Activa o desactiva el modo nocturno. |
| `INFO` | Muestra IP, señal WiFi, versión, ID y RAM libre (solo lectura). |
| `SONIDO` | Habilita o deshabilita la salida de sonido. |
| `VOLUMEN` | Ajusta el volumen del buzzer (0–30). |
| `OTA` | Verifica y descarga nuevo firmware si está disponible. |
| `ALARMAS` | Lista las alarmas; el botón del medio edita una (activar / hora / minuto). Los días, la etiqueta y la melodía se configuran desde la web o MQTT. |

## Submenú `ROTACION`

Agrupa los cuatro ajustes de rotación de apps. Dentro del submenú:

- **Botón del medio (pulsación corta):** pasa al siguiente campo.
- **Izquierda / Derecha:** ajustan el valor del campo actual.
- **Botón del medio (mantener):** aplica, guarda y sale.

| Campo | Ajuste | Rango |
| --- | --- | --- |
| `ROT` | Rotación automática de apps (`SI`/`NO`) | — |
| `TRA` | Duración de la transición (segundos) | 0.2 – 2.0 |
| `DES` | Velocidad de desplazamiento del texto | 10 – 100 |
| `APP` | Tiempo por app antes de cambiar (segundos) | 1 – 30 |

::: tip
Los valores de tiempo (`TRA`, `APP`) se muestran sin la letra "s"; el prefijo y la magnitud ya indican que son segundos.
:::

## Indicadores de estado (LEDs de esquina)

El reloj muestra dos píxeles de estado en las esquinas izquierdas de la matriz
para diagnosticar la conectividad de un vistazo. Cuando todo va bien, **ambos
están apagados**.

**Esquina superior izquierda — WiFi (rojo):**

| Patrón | Significado |
| --- | --- |
| Apagado | Conectado y con internet |
| Parpadeo lento | WiFi conectado pero sin acceso a internet (fallan las descargas, p. ej. el clima) |
| Parpadeo rápido | WiFi caído, reconectando |
| Encendido fijo | Modo AP — esperando configuración WiFi |

**Esquina inferior izquierda — Home Assistant / MQTT (amarillo):**

| Patrón | Significado |
| --- | --- |
| Apagado | Conectado, o MQTT no configurado |
| Parpadeo lento | MQTT configurado pero sin conexión con el broker |

::: tip
El indicador de WiFi se apaga en cuanto la conexión y las descargas vuelven a
funcionar, así que un parpadeo momentáneo solo señala un fallo pasajero.
:::
