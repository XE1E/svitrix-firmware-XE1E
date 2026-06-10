# Налаштування MQTT з Docker та Home Assistant

Посібник з налаштування брокера Mosquitto MQTT у Docker та його підключення до Home Assistant і SVITRIX-XE1E.

## Вимоги

- Сервер на Linux (Debian, Ubuntu тощо) зі встановленим Docker
- Home Assistant (встановлення через Docker або інше)
- SVITRIX-XE1E з прошивкою v0.4.0+

## 1. Встановлення Mosquitto у Docker

### 1.1 Створення каталогів

```bash
mkdir -p ~/mosquitto/config ~/mosquitto/data
```

### 1.2 Створення файлу конфігурації

```bash
nano ~/mosquitto/config/mosquitto.conf
```

Вміст:
```
listener 1883
allow_anonymous false
password_file /mosquitto/config/passwords
persistence true
persistence_location /mosquitto/data/
```

Зберегти: `Ctrl+O` → `Enter` → `Ctrl+X`

### 1.3 Створення порожнього файлу паролів

```bash
touch ~/mosquitto/config/passwords
chmod 666 ~/mosquitto/config/passwords
```

### 1.4 Запуск контейнера Mosquitto

```bash
docker run -d --name mosquitto --restart always -p 1883:1883 -v ~/mosquitto/config:/mosquitto/config -v ~/mosquitto/data:/mosquitto/data eclipse-mosquitto
```

> **Примітка:** Якщо ви працюєте від root, замініть `~` на `/root`

### 1.5 Перевірка, що контейнер працює

```bash
docker ps | grep mosquitto
```

Має відображатися стан "Up", а не "Restarting".

### 1.6 Створення користувача MQTT

```bash
docker exec -it mosquitto mosquitto_passwd /mosquitto/config/passwords svitrix
```

Вам запропонують двічі ввести пароль. Запам'ятайте його.

### 1.7 Перезапуск для застосування змін

```bash
docker restart mosquitto
```

### 1.8 Отримання IP-адреси сервера

```bash
hostname -I
```

Запишіть IP-адресу (наприклад: `192.168.1.12`).

---

## 2. Налаштування Home Assistant

1. Перейдіть до **Settings → Devices & Services**
2. Натисніть **Add Integration**
3. Знайдіть **MQTT**
4. Налаштуйте:

| Поле | Значення |
|------|----------|
| Broker | `<IP сервера>` (наприклад: 192.168.1.12) |
| Port | `1883` |
| Username | `svitrix` |
| Password | той, що ви встановили |

5. Натисніть **Submit**

---

## 3. Налаштування SVITRIX-XE1E

1. Відкрийте веб-інтерфейс пристрою: `http://<IP SVITRIX>/`
2. Перейдіть до **Settings → MQTT**
3. Налаштуйте:

| Поле | Значення |
|------|----------|
| Host | `<IP сервера>` (наприклад: 192.168.1.12) |
| Port | `1883` |
| User | `svitrix` |
| Password | той самий, що й у HA |
| HA Discovery | ✅ Увімкнено |

4. Натисніть **Save**
5. Перезавантажте пристрій

---

## 4. Перевірка з'єднання

### На SVITRIX
- Під час перезавантаження на екрані має ненадовго з'явитися "MQTT..."

### У Home Assistant
Перейдіть до **Settings → Devices & Services → MQTT → Devices**

Ви маєте побачити пристрій "SVITRIX" з такими сутностями:

**Lights (4):**
- Matrix (яскравість + колір RGB)
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
- Transition (автоперехід між застосунками)

**Selects (2):**
- Brightness Mode (Manual/Auto)
- Transition Effect

**Binary Sensors (3):**
- Button Left
- Button Select
- Button Right

---

## 5. Усунення несправностей

### Mosquitto не запускається (стан "Restarting")

Перегляньте логи:
```bash
docker logs mosquitto
```

**Поширена помилка:** `Unable to open pwfile`
- Рішення: Створіть порожній файл паролів (крок 1.3)

### Home Assistant не підключається

- Перевірте, що Mosquitto працює: `docker ps`
- Перевірте правильність IP-адреси
- Перевірте ім'я користувача та пароль
- Перевірте, що порт 1883 відкритий: `telnet <IP> 1883`

### SVITRIX не з'являється в HA

- Перевірте, що "HA Discovery" увімкнено на SVITRIX
- Перевірте, що облікові дані збігаються на обох пристроях
- Перезавантажте SVITRIX після зміни налаштувань MQTT

### Сенсори погоди показують "Unknown"

- Перевірте, що ключ WeatherAPI налаштовано на SVITRIX (Settings → Weather)
- Перевірте, що weatherData.valid дорівнює true (перевірте `/api/weather/data`)

---

## 6. Корисні команди

```bash
# Переглянути стан контейнера
docker ps | grep mosquitto

# Переглянути логи
docker logs mosquitto

# Перезапустити Mosquitto
docker restart mosquitto

# Зупинити Mosquitto
docker stop mosquitto

# Додати ще одного користувача
docker exec -it mosquitto mosquitto_passwd /mosquitto/config/passwords nuevo_usuario

# Видалити користувача
docker exec -it mosquitto mosquitto_passwd -D /mosquitto/config/passwords usuario

# Тест з'єднання (потрібен mosquitto-clients)
mosquitto_sub -h 192.168.1.12 -p 1883 -u svitrix -P password -t "#" -v
```

---

## 7. Додаткова конфігурація (необов'язково)

### Увімкнення WebSockets (порт 9001)

Відредагуйте `mosquitto.conf`:
```
listener 1883
listener 9001
protocol websockets
allow_anonymous false
password_file /mosquitto/config/passwords
persistence true
persistence_location /mosquitto/data/
```

Перестворіть контейнер з додатковим портом:
```bash
docker stop mosquitto && docker rm mosquitto
docker run -d --name mosquitto --restart always -p 1883:1883 -p 9001:9001 -v ~/mosquitto/config:/mosquitto/config -v ~/mosquitto/data:/mosquitto/data eclipse-mosquitto
```

### Увімкнення TLS/SSL

Потрібні сертифікати. Див. офіційну документацію Mosquitto.
