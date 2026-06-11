# Respaldo

La funcionalidad de respaldo y restauración está disponible en la [interfaz web](./webinterface) bajo la página **Respaldo** (`/backup`).

## Respaldo (Descargar)

Haz clic en **Descargar Respaldo** para guardar todos los archivos del sistema de archivos del dispositivo (iconos, melodías, paletas, apps personalizadas, configuración) como un solo archivo JSON.

::: tip Elegir la carpeta de descarga
Para que el navegador te pregunte **dónde guardar** (carpeta y nombre) al descargar el respaldo —o el PNG de la página *Vista*—, activa en tu navegador la opción **"Preguntar dónde guardar cada archivo antes de descargarlo"** (Chrome/Edge: *Configuración → Descargas*). Si accedes a la interfaz por **HTTPS** o **`localhost`**, la web abre directamente un diálogo de carpeta; por HTTP simple (la IP del reloj) el navegador no lo permite, así que la descarga usa tu carpeta de Descargas salvo que actives ese ajuste.
:::

## Restaurar (Subir)

Selecciona un archivo de respaldo previamente descargado para restaurar. Todos los archivos se subirán al dispositivo, y se reiniciará automáticamente después de una restauración exitosa.

::: warning
Restaurar un respaldo sobrescribirá los archivos existentes en el dispositivo. Asegúrate de que estás restaurando el respaldo correcto.
:::
