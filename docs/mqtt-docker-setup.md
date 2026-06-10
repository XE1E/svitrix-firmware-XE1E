# Configuración MQTT con Docker y Home Assistant

Guía para configurar el broker Mosquitto MQTT en Docker y conectarlo con Home Assistant y SVITRIX-XE1E.

## Requisitos

- Servidor Linux (Debian, Ubuntu, etc.) con Docker instalado
- Home Assistant (instalación Docker u otra)
- SVITRIX-XE1E con firmware v0.4.0+

## 1. Instalar Mosquitto en Docker

### 1.1 Crear directorios

```bash
mkdir -p ~/mosquitto/config ~/mosquitto/data
```

### 1.2 Crear archivo de configuración

```bash
nano ~/mosquitto/config/mosquitto.conf
```

Contenido:
```
listener 1883
allow_anonymous false
password_file /mosquitto/config/passwords
persistence true
persistence_location /mosquitto/data/
```

Guardar: `Ctrl+O` → `Enter` → `Ctrl+X`

### 1.3 Crear archivo de passwords vacío

```bash
touch ~/mosquitto/config/passwords
chmod 666 ~/mosquitto/config/passwords
```

### 1.4 Iniciar contenedor Mosquitto

```bash
docker run -d --name mosquitto --restart always -p 1883:1883 -v ~/mosquitto/config:/mosquitto/config -v ~/mosquitto/data:/mosquitto/data eclipse-mosquitto
```

> **Nota:** Si usas root, reemplaza `~` por `/root`

### 1.5 Verificar que está corriendo

```bash
docker ps | grep mosquitto
```

Debe mostrar estado "Up", no "Restarting".

### 1.6 Crear usuario MQTT

```bash
docker exec -it mosquitto mosquitto_passwd /mosquitto/config/passwords svitrix
```

Te pedirá escribir una contraseña 2 veces. Recuérdala.

### 1.7 Reiniciar para aplicar los cambios

```bash
docker restart mosquitto
```

### 1.8 Obtener la IP del servidor

```bash
hostname -I
```

Anota la IP (ej.: `192.168.1.12`).

---

## 2. Configurar Home Assistant

1. Ve a **Settings → Devices & Services**
2. Haz clic en **Add Integration**
3. Busca **MQTT**
4. Configura:

| Campo | Valor |
|-------|-------|
| Broker | `<IP del servidor>` (ej.: 192.168.1.12) |
| Port | `1883` |
| Username | `svitrix` |
| Password | el que configuraste |

5. Haz clic en **Submit**

---

## 3. Configurar SVITRIX-XE1E

1. Abre la interfaz web del dispositivo: `http://<IP del SVITRIX>/`
2. Ve a **Settings → MQTT**
3. Configura:

| Campo | Valor |
|-------|-------|
| Host | `<IP del servidor>` (ej.: 192.168.1.12) |
| Port | `1883` |
| User | `svitrix` |
| Password | la misma que en HA |
| HA Discovery | ✅ Activado |

4. Haz clic en **Save**
5. Reinicia el dispositivo

---

## 4. Verificar la conexión

### En SVITRIX
- Al reiniciar debe mostrar "MQTT..." en la pantalla brevemente

### En Home Assistant
Ve a **Settings → Devices & Services → MQTT → Devices**

Deberías ver un dispositivo "SVITRIX" con las siguientes entidades:

**Lights (4):**
- Matrix (brillo + color RGB)
- Indicator 1, 2, 3

**Sensors (~16):**
- Temperature (indoor)
- Humidity (indoor)
- Battery
- Illuminance
- WiFi Strength
- RAM
- Uptime
- IP Address
- Current App
- Version
- **Outdoor Temperature** (WeatherAPI)
- **Outdoor Humidity** (WeatherAPI)
- **Pressure** (WeatherAPI)
- **Air Quality Index** (WeatherAPI)
- **Weather Condition** (WeatherAPI)

**Buttons (4):**
- Dismiss notification
- Next App
- Previous App
- Start Update

**Switch (1):**
- Transition (auto-transición entre apps)

**Selects (2):**
- Brightness Mode (Manual/Auto)
- Transition Effect

**Binary Sensors (3):**
- Button Left
- Button Select
- Button Right

---

## 5. Resolución de problemas

### Mosquitto no arranca (estado "Restarting")

Ver los logs:
```bash
docker logs mosquitto
```

**Error común:** `Unable to open pwfile`
- Solución: Crear el archivo passwords vacío (paso 1.3)

### Home Assistant no conecta

- Verifica que Mosquitto está corriendo: `docker ps`
- Verifica que la IP es correcta
- Verifica el usuario y la contraseña
- Verifica que el puerto 1883 está abierto: `telnet <IP> 1883`

### SVITRIX no aparece en HA

- Verifica que "HA Discovery" está activado en SVITRIX
- Verifica que las credenciales coinciden en ambos
- Reinicia SVITRIX después de cambiar la configuración MQTT

### Los sensores del clima muestran "Unknown"

- Verifica que la clave de WeatherAPI está configurada en SVITRIX (Settings → Weather)
- Verifica que weatherData.valid es true (comprueba `/api/weather/data`)

---

## 6. Comandos útiles

```bash
# Ver estado del contenedor
docker ps | grep mosquitto

# Ver logs
docker logs mosquitto

# Reiniciar Mosquitto
docker restart mosquitto

# Parar Mosquitto
docker stop mosquitto

# Agregar otro usuario
docker exec -it mosquitto mosquitto_passwd /mosquitto/config/passwords nuevo_usuario

# Eliminar usuario
docker exec -it mosquitto mosquitto_passwd -D /mosquitto/config/passwords usuario

# Test de conexión (requiere mosquitto-clients)
mosquitto_sub -h 192.168.1.12 -p 1883 -u svitrix -P password -t "#" -v
```

---

## 7. Configuración adicional (opcional)

### Habilitar WebSockets (puerto 9001)

Editar `mosquitto.conf`:
```
listener 1883
listener 9001
protocol websockets
allow_anonymous false
password_file /mosquitto/config/passwords
persistence true
persistence_location /mosquitto/data/
```

Recrear el contenedor con el puerto adicional:
```bash
docker stop mosquitto && docker rm mosquitto
docker run -d --name mosquitto --restart always -p 1883:1883 -p 9001:9001 -v ~/mosquitto/config:/mosquitto/config -v ~/mosquitto/data:/mosquitto/data eclipse-mosquitto
```

### Habilitar TLS/SSL

Requiere certificados. Consulta la documentación oficial de Mosquitto.
