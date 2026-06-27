# Інтеграція з Home Assistant

SVITRIX підтримує автоматичне виявлення в Home Assistant через MQTT. Після увімкнення пристрій автоматично реєструється в HA з 66 керованими сутностями.

## Вимоги

- Налаштований MQTT-брокер (Mosquitto, EMQX тощо)
- Home Assistant з увімкненою інтеграцією MQTT
- SVITRIX, підключений до того ж MQTT-брокера

## Налаштування

### 1. У SVITRIX

Перейдіть до **Settings > MQTT** у веб-інтерфейсі:

| Поле | Опис |
|-------|-------------|
| Host | IP або ім'я хоста MQTT-брокера |
| Port | Порт (за замовчуванням: 1883) |
| User / Password | Облікові дані брокера |
| Prefix | Префікс MQTT-топіків (за замовчуванням: `svitrix`) |
| **HA Discovery** | Увімкніть для автоматичного виявлення |

### 2. У Home Assistant

Інтеграцію MQTT має бути налаштовано. Якщо ви використовуєте надбудову Mosquitto від HA, усе вже готове. Для зовнішніх брокерів:

```yaml
# configuration.yaml
mqtt:
  broker: 192.168.1.100
  port: 1883
  username: your_user
  password: your_password
```

Після увімкнення HA Discovery у SVITRIX та перезавантаження сутності з'являться автоматично у **Settings > Devices & Services > MQTT**.

---

## Доступні сутності (всього 66)

### Світло / Lights (5)

| Сутність | Опис | Керування |
|--------|-------------|----------|
| **Matrix** | Основний LED-дисплей | Увімк./Вимк., яскравість (0-255), RGB-колір |
| **Indicator 1** | Верхній правий індикаторний LED | Увімк./Вимк., RGB-колір |
| **Indicator 2** | Середній правий індикаторний LED | Увімк./Вимк., RGB-колір |
| **Indicator 3** | Нижній правий індикаторний LED | Увімк./Вимк., RGB-колір |
| **Night color** | Колір тексту в нічному режимі | RGB-колір |

### Перемикачі / Switches (14)

| Сутність | Опис |
|--------|-------------|
| **Transition** | Увімкнути/вимкнути автоматичне перемикання додатків |
| **Night mode** | Активувати/деактивувати нічний режим |
| **Night block transition** | Блокувати переходи під час нічного режиму |
| **Sound enabled** | Увімкнути/вимкнути звуки зуммера |
| **Show time app** | Показати/приховати додаток часу |
| **Show date app** | Показати/приховати додаток дати |
| **Show temperature app** | Показати/приховати додаток температури |
| **Show humidity app** | Показати/приховати додаток вологості |
| **Show battery app** | Показати/приховати додаток батареї |
| **Show outdoor temp** | Показати/приховати зовнішню температуру (погода) |
| **Show outdoor humidity** | Показати/приховати зовнішню вологість (погода) |
| **Show pressure** | Показати/приховати атмосферний тиск (погода) |
| **Show air quality** | Показати/приховати індекс якості повітря (погода) |
| **Show UV index** | Показати/приховати UV-індекс (погода) |

### Числа / Numbers (14)

| Сутність | Діапазон | Опис |
|--------|-------|-------------|
| **Night brightness** | 1-50 | Яскравість під час нічного режиму |
| **Sound volume** | 0-30 | Гучність зуммера |
| **Time per app** | 1-60с | Тривалість показу кожного додатку в ротації |
| **Scroll speed** | 20-200мс | Швидкість прокрутки тексту |
| **Clock duration** | 1-300с | Тривалість додатку часу |
| **Date duration** | 1-60с | Тривалість додатку дати |
| **Temperature duration** | 1-60с | Тривалість додатку температури |
| **Humidity duration** | 1-60с | Тривалість додатку вологості |
| **Battery duration** | 1-60с | Тривалість додатку батареї |
| **Outdoor temp duration** | 1-60с | Тривалість зовнішньої температури |
| **Outdoor humidity duration** | 1-60с | Тривалість зовнішньої вологості |
| **Pressure duration** | 1-60с | Тривалість атмосферного тиску |
| **Air quality duration** | 1-60с | Тривалість якості повітря |
| **UV index duration** | 1-60с | Тривалість UV-індексу |

> **Примітка:** **Числа тривалості для кожного додатку** та **перемикачі видимості** (Show time/date/temp/hum/bat + видимість погоди) тепер відображають і керують **фактичною ротацією** пристрою (єдиним списком ротації), а не застарілими значеннями за замовчуванням — тож те, що показує HA, збігається з тим, що реально робить годинник, а зміна застосовується до всіх екземплярів цього типу додатку. Тонке налаштування для кожного окремого екземпляра залишається в редакторі ротації у веб-інтерфейсі. Числа тривалості подаються як числове **поле** (значення завжди видиме), а не повзунок.

### Селектори / Selects (3)

| Сутність | Варіанти | Опис |
|--------|---------|-------------|
| **Brightness mode** | Manual, Auto | Режим яскравості (ручний або за датчиком освітленості) |
| **Transition effect** | 14 ефектів | Ефект переходу між додатками |
| **Background effect** | 21 варіант | Фоновий ефект (None, Fade, Matrix, Plasma, Fire тощо) |

### Кнопки / Buttons (8)

| Сутність | Дія |
|--------|--------|
| **Dismiss notification** | Закрити поточне сповіщення |
| **Start Update** | Розпочати OTA-оновлення прошивки |
| **Next app** | Перейти до наступного додатку |
| **Previous app** | Перейти до попереднього додатку |
| **Reboot** | Перезавантажити пристрій |
| **Play test sound** | Відтворити тестовий звуковий сигнал |
| **Snooze alarm** | Відкласти активний будильник |
| **Dismiss alarm** | Скасувати активний будильник |

### Сенсори / Sensors (17-18)

| Сутність | Одиниця | Опис |
|--------|------|-------------|
| **Current app** | — | Назва активного додатку |
| **Device topic** | — | MQTT-префікс пристрою |
| **Temperature** | °C | Температура навколишнього середовища (внутрішній сенсор) |
| **Humidity** | % | Відносна вологість (внутрішній сенсор) |
| **Illuminance** | lx | Освітленість (датчик LDR) |
| **WiFi signal** | dB | Рівень WiFi-сигналу |
| **Firmware version** | — | Версія прошивки |
| **Free RAM** | bytes | Доступна пам'ять купи (heap) |
| **Uptime** | s | Час з моменту останнього перезавантаження |
| **IP address** | — | IP-адреса пристрою |
| **Battery** | % | Рівень заряду батареї (лише Ulanzi TC001) |
| **Outdoor temperature** | °C | Зовнішня температура (WeatherAPI) |
| **Outdoor humidity** | % | Зовнішня вологість (WeatherAPI) |
| **Pressure** | hPa | Атмосферний тиск (WeatherAPI) |
| **Air quality** | AQI | Індекс якості повітря (WeatherAPI) |
| **Weather condition** | — | Поточні погодні умови (WeatherAPI) |
| **UV index** | — | UV-індекс (WeatherAPI) |
| **Next alarm** | — | Наступний запланований будильник (HH:MM або "None") |

#### Причина останнього скидання (`reset_reason`)

Корисне навантаження періодичного топіка `<prefix>/stats` (а також `GET /api/stats`) містить поле **`reset_reason`** з причиною останнього завантаження: `poweron` (звичайне ввімкнення живлення), `ext` (скидання зовнішнім пином), `software` (програмне/OTA-скидання), `panic` (виняток/збій), `int_wdt` / `task_wdt` / `wdt` (вотчдог), `brownout` (низька напруга), `deepsleep` (пробудження з глибокого сну), `sdio` та `unknown` (запасний варіант).

Це не автоматично виявлена сутність; задайте її як MQTT-сенсор, щоб отримувати сповіщення про неочікувані перезавантаження (вотчдог, panic, brownout) без послідовної консолі:

```yaml
# configuration.yaml
mqtt:
  - sensor:
      name: "SVITRIX Reset Reason"
      state_topic: "svitrix/stats"   # use your MQTT prefix
      value_template: "{{ value_json.reset_reason }}"
      icon: mdi:restart-alert
```

### Бінарні сенсори / Binary Sensors (4)

| Сутність | Опис |
|--------|-------------|
| **Button left** | Стан лівої кнопки |
| **Button middle** | Стан середньої кнопки |
| **Button right** | Стан правої кнопки |
| **Alarm ringing** | Вказує, чи лунає будильник |

---

## Приклади автоматизацій

### Вимкнути звук, коли увімкнено телевізор

```yaml
alias: "SVITRIX - Mute with TV"
trigger:
  - platform: state
    entity_id: media_player.living_room_tv
    to: "on"
action:
  - service: switch.turn_off
    target:
      entity_id: switch.svitrix_sound_enabled
```

### Активувати нічний режим зі сценою

```yaml
alias: "SVITRIX - Movie mode"
trigger:
  - platform: state
    entity_id: input_boolean.movie_mode
    to: "on"
action:
  - service: switch.turn_on
    target:
      entity_id: switch.svitrix_night_mode
  - service: number.set_value
    target:
      entity_id: number.svitrix_night_brightness
    data:
      value: 5
  - service: light.turn_on
    target:
      entity_id: light.svitrix_night_color
    data:
      rgb_color: [255, 50, 0]
```

### Сповіщення про натискання кнопки

```yaml
alias: "SVITRIX - Middle button pressed"
trigger:
  - platform: state
    entity_id: binary_sensor.svitrix_button_middle
    to: "on"
action:
  - service: notify.mobile_app
    data:
      message: "Middle button pressed on SVITRIX"
```

### Автоматична яскравість за часом

```yaml
alias: "SVITRIX - Brightness by schedule"
trigger:
  - platform: time
    at: "22:00:00"
action:
  - service: select.select_option
    target:
      entity_id: select.svitrix_brightness_mode
    data:
      option: "Manual"
  - service: light.turn_on
    target:
      entity_id: light.svitrix_matrix
    data:
      brightness: 30
```

### Індикатор стану сигналізації

```yaml
alias: "SVITRIX - Alarm indicator"
trigger:
  - platform: state
    entity_id: alarm_control_panel.home
action:
  - choose:
      - conditions:
          - condition: state
            entity_id: alarm_control_panel.home
            state: "armed_away"
        sequence:
          - service: light.turn_on
            target:
              entity_id: light.svitrix_indicator_1
            data:
              rgb_color: [255, 0, 0]
      - conditions:
          - condition: state
            entity_id: alarm_control_panel.home
            state: "disarmed"
        sequence:
          - service: light.turn_on
            target:
              entity_id: light.svitrix_indicator_1
            data:
              rgb_color: [0, 255, 0]
```

### Надсилання сповіщення через MQTT

Для більш просунутих сповіщень використовуйте MQTT-топік напряму:

```yaml
alias: "SVITRIX - Doorbell notification"
trigger:
  - platform: state
    entity_id: binary_sensor.doorbell
    to: "on"
action:
  - service: mqtt.publish
    data:
      topic: "svitrix/notify"
      payload: >
        {
          "text": "Someone at the door",
          "icon": "door",
          "color": "#FFAA00",
          "duration": 10,
          "sound": "doorbell"
        }
```

---

## Інтеграція з Google Calendar

Відображайте події, річниці та завдання з Google Calendar на SVITRIX, використовуючи Home Assistant як міст.

### Вимоги

- Home Assistant з працюючою інтеграцією MQTT
- Обліковий запис Google з доступом до Google Calendar

### 1. Налаштуйте Google Calendar у Home Assistant

1. Перейдіть до **Settings > Devices & Services > Add Integration**
2. Знайдіть **Google Calendar**
3. Пройдіть процес OAuth-автентифікації з вашим обліковим записом Google
4. Виберіть календарі, які хочете імпортувати

Після налаштування ви матимете сутності на кшталт:
- `calendar.my_personal_calendar`
- `calendar.birthdays`
- `calendar.work`

### 2. Перевірте вручну

Перш ніж автоматизувати, перевірте надсиланням події вручну з **Developer Tools > Services**:

```yaml
service: mqtt.publish
data:
  topic: "svitrix/notify"
  payload: |
    {
      "text": "Calendar test",
      "icon": "6741",
      "duration": 10,
      "color": "#00BFFF"
    }
```

Якщо вона з'явиться на SVITRIX, MQTT-з'єднання працює коректно.

### 3. Автоматизації

#### Показ наступної події як постійного додатку

```yaml
automation:
  - alias: "SVITRIX - Next calendar event"
    trigger:
      - platform: time_pattern
        minutes: "/15"  # update every 15 min
      - platform: state
        entity_id: calendar.my_calendar
    condition:
      - condition: state
        entity_id: calendar.my_calendar
        state: "on"  # there's an active or upcoming event
    action:
      - service: mqtt.publish
        data:
          topic: "svitrix/custom/calendar"
          payload: >
            {
              "text": "{{ state_attr('calendar.my_calendar', 'message') }}",
              "icon": "6741",
              "duration": 10,
              "color": "#00BFFF"
            }
```

#### Сповіщення про день народження/річницю

```yaml
automation:
  - alias: "SVITRIX - Daily anniversaries"
    trigger:
      - platform: time
        at: "08:00:00"
    condition:
      - condition: state
        entity_id: calendar.birthdays
        state: "on"
    action:
      - service: mqtt.publish
        data:
          topic: "svitrix/notify"
          payload: >
            {
              "text": "{{ state_attr('calendar.birthdays', 'message') }}",
              "icon": "955",
              "duration": 30,
              "color": "#FF69B4",
              "sound": "birthday"
            }
```

#### Нагадування про зустріч (за 15 хв)

```yaml
automation:
  - alias: "SVITRIX - Meeting reminder"
    trigger:
      - platform: calendar
        event: start
        entity_id: calendar.work
        offset: "-0:15:00"
    action:
      - service: mqtt.publish
        data:
          topic: "svitrix/notify"
          payload: >
            {
              "text": "In 15 min: {{ trigger.calendar_event.summary }}",
              "icon": "7956",
              "duration": 60,
              "color": "#FFA500",
              "sound": "chime"
            }
```

### 4. Рекомендовані іконки

| Іконка | ID | Використання |
|------|-----|-----|
| Calendar | 6741 | Загальні події |
| Cake | 955 | Дні народження |
| Gift | 52 | Річниці |
| Meeting | 7956 | Робота/зустрічі |
| Check | 51167 | Завдання |
| Alarm | 5765 | Нагадування |

Більше іконок можна знайти в [бібліотеці іконок LaMetric](https://developer.lametric.com/icons).

### 5. Видалення додатку календаря

Щоб прибрати додаток календаря з ротації:

```yaml
service: mqtt.publish
data:
  topic: "svitrix/custom/calendar"
  payload: "{}"
```

---

## Інтеграція з Alexa та Google Home

Використовуйте SVITRIX за допомогою голосових команд через Home Assistant.

### Вимоги

- Home Assistant з працюючою інтеграцією MQTT
- **Для Alexa:** [Alexa Media Player](https://github.com/custom-components/alexa_media_player) (HACS) або Home Assistant Cloud
- **Для Google:** нативна інтеграція Google Assistant або Home Assistant Cloud

### Початкове налаштування

#### 1. Надайте сутності асистентам

Щоб керувати SVITRIX голосом, надайте доступ до цих сутностей:

```yaml
# configuration.yaml
cloud:
  alexa:
    filter:
      include_entities:
        - light.svitrix_matrix
        - switch.svitrix_night_mode
        - switch.svitrix_sound_enabled
  google_assistant:
    filter:
      include_entities:
        - light.svitrix_matrix
        - switch.svitrix_night_mode
        - switch.svitrix_sound_enabled
```

#### 2. Створіть input_boolean для рутин

Щоб активувати автоматизації голосом:

```yaml
# configuration.yaml
input_boolean:
  good_morning:
    name: Good morning
    icon: mdi:weather-sunny
  show_message:
    name: Show message
    icon: mdi:message
```

Надайте ці `input_boolean` для Alexa/Google. Потім створіть рутини:
- **Alexa:** "When I say 'Good morning'" → активувати `input_boolean.good_morning`
- **Google:** "When I say 'Good morning'" → активувати `input_boolean.good_morning`

### Доступні голосові команди

Після налаштування ви можете використовувати:

| Команда | Дія |
|---------|--------|
| "Alexa, turn on the clock" | Увімкнути матрицю |
| "Hey Google, turn off the clock" | Вимкнути матрицю |
| "Alexa, set clock brightness to 50%" | Налаштувати яскравість |
| "Hey Google, activate night mode" | Активувати нічний режим |
| "Alexa, Good morning" | Виконати ранкову рутину |

### Блупринти для голосових асистентів

| Блупринт | Опис | Імпорт |
|-----------|-------------|--------|
| **Button to announcement** | Фізична кнопка → Alexa/Google озвучує | [![Import](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https://xe1e.github.io/svitrix-firmware-XE1E/blueprints/svitrix_button_announcement.yaml) |
| **Voice notification** | Скрипт для надсилання тексту на SVITRIX | [![Import](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https://xe1e.github.io/svitrix-firmware-XE1E/blueprints/svitrix_voice_notification.yaml) |
| **Morning routine** | "Good morning" → погода + календар | [![Import](https://my.home-assistant.io/badges/blueprint_import.svg)](https://my.home-assistant.io/redirect/blueprint_import/?blueprint_url=https://xe1e.github.io/svitrix-firmware-XE1E/blueprints/svitrix_morning_routine.yaml) |

---

## Усунення несправностей

### Сутності не з'являються в HA

1. Переконайтеся, що MQTT підключено (іконка WiFi+MQTT на SVITRIX)
2. Підтвердіть, що "HA Discovery" увімкнено в Settings > MQTT
3. Перезавантажте SVITRIX після увімкнення discovery
4. Перевірте журнали HA у **Settings > System > Logs**

### Сутності відображаються як "unavailable"

- Пристрій може бути відключений або перезавантажується
- Перевірте MQTT-з'єднання з обох боків

### Зміни не відображаються

- Деякі зміни потребують публікації стану від SVITRIX (за замовчуванням кожні 5 секунд)
- Щоб примусово оновити негайно, перезавантажте пристрій

### Сирітські сутності (від попередніх версій)

Якщо ви бачите сутності на кшталт "Clock Color", "Time Color", які не відповідають:
1. Перейдіть до **HA > Settings > Devices & Services > MQTT**
2. Знайдіть свій пристрій SVITRIX
3. Видаліть невідповідні сутності вручну

Або примусово виконайте повне повторне виявлення:
1. У веб-інтерфейсі SVITRIX: вимкніть "HA Discovery", збережіть
2. У HA: видаліть весь пристрій SVITRIX
3. У веб-інтерфейсі SVITRIX: знову увімкніть "HA Discovery", збережіть

---

## Дивіться також

- [MQTT/HTTP API](api.md) — Розширені команди через MQTT
- [Налаштування MQTT з Docker](mqtt-docker-setup.md) — Встановлення Mosquitto
