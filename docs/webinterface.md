# Interfaz Web

SVITRIX usa una moderna Aplicación de Página Única (SPA) construida con Preact como su interfaz web. La SPA se sirve desde el sistema de archivos LittleFS del dispositivo y se comunica con el firmware vía API REST.

Una vez que SVITRIX está conectado a tu red WiFi, accede a la interfaz web ingresando la dirección IP del dispositivo en tu navegador (puerto 80). La dirección IP se muestra en la matriz en cada arranque.

## Páginas

| Página | Ruta | Descripción |
|--------|------|-------------|
| **Pantalla** | `/` | Vista en vivo de la matriz LED 32x8 con navegación de apps (anterior/siguiente), descarga PNG y grabación GIF. |
| **Configuración** | `/settings` | Configuración del dispositivo dividida en secciones independientes, cada una con su propio botón Guardar: WiFi, Red, MQTT, NTP/Zona horaria, Autenticación, Pantalla, Apps, Hora y Fecha, Sonido, Enviar Notificación y Selector de Iconos. Incluye alternador de tema oscuro/claro. |
| **Data Fetcher** | `/datafetcher` | Configura fuentes de datos HTTP externas que automáticamente obtienen y muestran datos en la matriz. Ver [Data Fetcher](./datafetcher) para detalles. |
| **Archivos** | `/files` | Administrador de archivos integrado para navegar, subir, descargar, editar y eliminar archivos en el dispositivo (iconos, melodías, apps personalizadas, paletas). |
| **Respaldo** | `/backup` | Descarga todos los archivos del dispositivo como un respaldo JSON, o restaura desde un respaldo previamente descargado. |
| **Actualizar** | `/update` | Sube firmware (.bin) para actualización OTA. El dispositivo se reinicia automáticamente después de una subida exitosa. |

## Primera Configuración (Modo AP)

Cuando SVITRIX no puede conectarse a una red WiFi guardada, crea su propio punto de acceso:

| Parámetro | Valor |
|-----------|-------|
| Nombre de red | `svitrix_XXXXX` |
| Contraseña | `12345678` |

Conéctate a esta red y abre **http://192.168.4.1** en tu navegador. Aparecerá una página mínima de configuración WiFi con escaneo de redes y conexión. Después de conectarte a tu WiFi doméstico, el dispositivo se reinicia y la SPA completa estará disponible.

## Despliegue de la SPA

La interfaz web se almacena por separado del firmware en la partición del sistema de archivos LittleFS. Después de flashear el firmware, sube los archivos de la SPA al dispositivo:

```bash
cd web && npm run upload
```

Esto compila la SPA y la sube al directorio raíz de LittleFS del dispositivo. El bundle de la SPA es aproximadamente 18 KB (comprimido con gzip) e incluye las 6 páginas.

::: tip
Una vez subida, la SPA persiste entre actualizaciones de firmware. Solo necesitas volver a subir la SPA cuando la interfaz web misma se actualice.
:::

## Guía de Configuración

La página de Configuración está organizada en secciones independientes. La mayoría de opciones se **guardan automáticamente** al cambiarlas — toggles y selectores aplican al instante, mientras que sliders y colores esperan 500ms después del último cambio para guardar.

**Secciones con guardado manual** (requieren botón Guardar): WiFi, Red, MQTT, NTP, Autenticación y API del clima — estas manejan credenciales sensibles donde un guardado accidental podría causar problemas de conexión.

Un **alternador de tema oscuro/claro** (☀/☽) está disponible en la esquina superior derecha de la barra de navegación. Tu preferencia se guarda en el navegador.

### Barra de Estadísticas

Una barra de solo lectura en la parte superior mostrando información del dispositivo en tiempo real: versión del firmware, RAM libre, intensidad de señal WiFi, luz ambiental (lux), tiempo de actividad, temperatura, humedad y brillo actual.

### WiFi

Configura hasta **3 redes WiFi**. El dispositivo intenta conectarse a cada una en orden. Escanea las redes disponibles, selecciona una (o escríbela manualmente) e ingresa la contraseña. Puedes guardar sin reiniciar o guardar y reiniciar para aplicar los cambios.

### Red

Habilita **IP Estática** para configurar una dirección IP fija, puerta de enlace, subred y servidor DNS en lugar de DHCP.

### MQTT

Conéctate a un broker MQTT para integración con Home Assistant y control remoto:
- **Broker** — hostname o IP de tu broker MQTT
- **Puerto** — por defecto 1883
- **Usuario / Contraseña** — credenciales del broker
- **Prefijo** — prefijo del topic MQTT (por defecto: ID del dispositivo)
- **Home Assistant Discovery** — habilita auto-descubrimiento de entidades del dispositivo en HA

### NTP y Zona Horaria

- **Servidor NTP** — servidor de tiempo (por defecto: `time.cloudflare.com`)
- **Zona horaria** — cadena de zona horaria POSIX (encuentra la tuya en [posix_tz_db](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv))

### API del Clima

Configura el origen de los datos meteorológicos usados por las apps del clima (Temp. Exterior, Humedad Exterior, Presión, Calidad del Aire, UV):

- **Clave API** — clave de [weatherapi.com](https://www.weatherapi.com) (registro gratuito)
- **Ubicación** — método para fijar la localización: por **ciudad** (nombre), por **coordenadas** (latitud/longitud), **auto por IP** (detección automática) o por **estación PWS** (ID de estación meteorológica personal)
- **Intervalo de actualización** — cada cuánto se vuelven a obtener los datos
- **Obtener ahora** — botón para forzar una actualización inmediata y comprobar la configuración

### Autenticación

Ver la sección [Autenticación](#autenticación) abajo.

### Modo Nocturno

Programa un modo de bajo brillo y un solo color para uso nocturno (ej. reloj de dormitorio):

- **Habilitar Modo Nocturno** — activa/desactiva la función
- **Inicio / Fin** — rango de tiempo (ej. 21:00 a 06:00, soporta cruzar medianoche)
- **Brillo Nocturno** — brillo de pantalla durante horas nocturnas (1–50)
- **Color Nocturno** — color de texto durante modo nocturno (por defecto: rojo — más fácil para los ojos)
- **Bloquear Auto-Transición** — cuando está marcado, las apps no ciclan automáticamente; usa botones para navegar

Durante el tiempo programado, la pantalla reduce su brillo a la configuración establecida y todo el texto se renderiza en el color nocturno elegido. Cuando termina la ventana de tiempo, la configuración normal se restaura automáticamente.

### Pantalla

- **Energía de Matriz** — enciende/apaga la matriz LED
- **Brillo Automático** — ajusta automáticamente el brillo basado en luz ambiental. Al activarlo se muestran dos sub-controles que definen el rango: **Brillo Mínimo** (1–50) y **Brillo Máximo** (0–255)
- **Brillo** — nivel de brillo manual, expresado como porcentaje (0–100 %)
- **Gamma** — curva de corrección gamma (0.5–3.0)
- **Mayúsculas** — fuerza todo el texto a mayúsculas
- **Color de Texto** — color de texto por defecto para todas las apps
- **Efecto de Fondo** — selecciona un efecto visual que se dibuja detrás de las apps (Ninguno por defecto)
- **Corrección de Color** — tinte global que se multiplica sobre toda la pantalla (lo aplica FastLED al refrescar). El **blanco** es el valor neutro (sin cambio); solo puede **atenuar** canales de color, nunca añadirlos. Úsalo para corregir un tono dominante del panel. El botón **Restablecer** vuelve a blanco.
- **Temperatura de Color** — mismo tipo de tinte global, pensado para entibiar o enfriar la imagen. Incluye atajos **Cálido / Neutro / Frío**; **Neutro** (blanco) equivale a desactivarlo. Elige colores saturados para notar el efecto.

### Apps

La página de **Apps** contiene una lista unificada de rotación que controla **todas** las apps y efectos que se muestran en el ciclo:

**Rotación Unificada** — Una lista con arrastrar y soltar (drag-and-drop) que incluye:
- **Apps nativas** — Hora, Fecha, Temperatura, Humedad, Batería
- **Apps del clima** — Temp. Exterior, Humedad Exterior, Presión, Calidad del Aire, UV
- **Apps personalizadas** — creadas vía MQTT, HTTP o Data Fetcher
- **Efectos** — efectos visuales independientes (sin texto)

Cada elemento en la rotación se puede expandir (▼) para mostrar sus configuraciones individuales:
- **Toggle** — activa/desactiva el elemento
- **Duración** — tiempo de visualización por elemento, ajustable con un control **−/+** (toca = ±1 s, mantén pulsado = ±10 s). 0 = usar la duración por defecto (configurable abajo)
- **Color** — color de texto personalizado (0 = usar color por defecto)
- **Icono** — sobrescribe el icono del elemento con un ID de icono o nombre de archivo (no disponible para Hora/Fecha; deja vacío para usar el icono por defecto que se muestra como placeholder)
- **Celsius** — solo en Temperatura y Temp. Exterior: muestra °C cuando está activo, °F cuando está apagado
- **Offset** — solo en Temperatura: corrección de temperatura aplicada a la lectura del sensor interno (de −15 a +5°)
- **Auto color** — solo en Calidad del Aire (AQI) y UV: cuando está activo, el color se asigna dinámicamente según el nivel (anula el color fijo/por elemento)

Arrastra cualquier fila para reordenarla. Los cambios se guardan al instante y persisten entre reinicios.

**Agregar elementos:**
- Botón **+ Agregar** — abre un modal para agregar apps nativas, del clima o efectos
- Las apps personalizadas se agregan automáticamente cuando se crean vía MQTT/HTTP/DataFetcher

**Comportamiento global:**
- **Auto Transición** — cicla automáticamente a través de las apps
- **Efecto de Transición** — efecto visual al cambiar apps (Ninguno, Deslizar, Atenuar, Zoom, etc.)
- **Velocidad de Transición** — qué tan rápido reproduce la animación de transición (100–2000ms)
- **Velocidad de Desplazamiento** — velocidad de desplazamiento de texto para texto largo
- **Duración por defecto** — segundos que se muestra cada app cuando su duración por elemento es 0 (1–30 s; equivale a `ROTACION → APP` del menú físico)
- **Bloquear Navegación** — deshabilita navegación con botones entre apps

### Hora y Fecha

- **Formato de Hora / Formato de Fecha** — cadenas de formato strftime (ej., `%H:%M`, `%d.%m.%y`)
- **Modo de Hora** — estilo de visualización, 7 opciones:
  1. **Texto simple (día abajo)**
  2. **Calendario (día abajo)**
  3. **Calendario (día arriba)**
  4. **Calendario Alt (día abajo)**
  5. **Calendario Alt (día arriba)**
  6. **Dígitos grandes**
  7. **Binario**
  (con formatos de hora que incluyen segundos solo están disponibles Texto simple y Binario)
- **Comenzar en Lunes** — la semana comienza en lunes en lugar de domingo
- **Color de Hora / Fecha** — colores individuales para apps de hora y fecha
- **Mostrar Día de Semana** — muestra barra indicadora de día de semana
- **Color Día Activo / Inactivo** — colores para puntos de día de semana
- **Color Encabezado / Texto / Cuerpo de Calendario** — colores para el cuadro de calendario

### Sonido

- **Sonido Habilitado** — habilita/deshabilita el buzzer
- **Volumen** — nivel de volumen del buzzer (0–30)

### Enviar Notificación

Envía un mensaje único a la pantalla:
- **Texto** — mensaje a mostrar (requerido)
- **Icono** — ID de icono o nombre de archivo de `/ICONS/`
- **Disposición** — posiciona el icono a la izquierda o derecha (solo visible cuando hay icono)
- **Hold (indefinido)** — mantiene la notificación hasta presionar Dismiss o el botón central
- **Duración** — cuánto tiempo se muestra la notificación (1–60s, solo cuando Hold está desactivado)
- **Arcoíris** — cicla el texto a través de colores del arcoíris
- **Color** — color de texto (cuando arcoíris está apagado)
- **Sonido** — nombre de archivo de melodía RTTTL en `/MELODIES/` (sin extensión)
- **RTTTL** — cadena de melodía en [formato RTTTL](https://en.wikipedia.org/wiki/Ring_Tone_Text_Transfer_Language) directo

::: tip
Si no especificas icono, el texto usa los 32 píxeles completos de la pantalla. Los textos largos hacen scroll automáticamente.
:::

Ver [Sonidos](./sounds) para crear archivos de melodía.

### Alarmas

La pestaña de **Alarmas** permite gestionar despertadores y recordatorios:
- **Agregar / Editar alarma** — define la **hora**, los **días** de la semana, una **etiqueta** opcional, la **melodía** que suena y la opción de **posponer** (snooze)
- **Una sola vez** — marca la alarma para que suene una única vez y se desactive después
- **Indicador de alarma** — activa/desactiva el pequeño indicador que muestra en la matriz que hay una alarma programada

Las alarmas funcionan incluso sin WiFi gracias al reloj de tiempo real (RTC).

### Iconos

**Selector de Iconos** — Descarga iconos de la [biblioteca de iconos LaMetric](https://developer.lametric.com/icons):
1. Ingresa el número de ID del icono
2. Haz clic en **Vista Previa** para verlo
3. Haz clic en **Descargar** para guardarlo en la carpeta `/ICONS/` del dispositivo

**Galería de Iconos Guardados** — Debajo del selector, una galería muestra todos los iconos almacenados en el dispositivo:
- Vista en grid con imagen y número de ID
- Botón de eliminar (×) al pasar el mouse
- Botón "Actualizar" para recargar la lista

### Acciones

- **Guardar Toda la Configuración** — guarda todas las configuraciones de pantalla, MQTT y clima de una vez (útil como respaldo manual o si el auto-guardado falló)
- **Restablecer Valores** — restaura todas las configuraciones a valores de fábrica (requiere confirmación)
- **Reiniciar** — reinicia el dispositivo (requiere confirmación)

## Autenticación

Puedes establecer un nombre de usuario y contraseña en Configuración → Autenticación. Cuando está configurado, cada página, llamada API y la app SVITRIX requerirán estas credenciales. Deja ambos campos vacíos para deshabilitar la autenticación.

::: warning
No pierdas tus credenciales de autenticación — de lo contrario necesitarás hacer un reset de fábrica del dispositivo.
:::
