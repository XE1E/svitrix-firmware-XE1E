# Backup

The backup and restore functionality is available in the [web interface](./webinterface) under the **Backup** page (`/backup`).

## Backup (Download)

Click **Download Backup** to save all files from the device's filesystem (icons, melodies, palettes, custom apps, configuration) as a single JSON file.

::: tip Choosing the download folder
To make the browser prompt for **where to save** (folder and name) when downloading the backup — or the PNG from the *Screen* page — enable your browser's **"Ask where to save each file before downloading"** option (Chrome/Edge: *Settings → Downloads*). When you open the interface over **HTTPS** or **`localhost`**, the web app opens a folder dialog directly; over plain HTTP (the clock's IP) the browser disallows it, so the download goes to your default Downloads folder unless that setting is on.
:::

## Restore (Upload)

Select a previously downloaded backup file to restore. All files will be uploaded to the device, and it will reboot automatically after a successful restore.

::: warning
Restoring a backup will overwrite existing files on the device. Make sure you're restoring the correct backup.
:::
