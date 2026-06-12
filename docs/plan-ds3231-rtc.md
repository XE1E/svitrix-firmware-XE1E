# Plan de Implementación — RTC DS3231

> **Estado:** Diseño aprobado, pendiente de hardware
> **Fecha:** 2026-06-12
> **Disparador:** Implementar cuando llegue el módulo DS3231 (ZS-042 / MH-RTC u otro)
> **Alcance:** Driver `IRtcProvider` dedicado para el DS3231, seleccionable por flag de compilación. Sin romper el soporte DS1307 existente.

---

## 1. Contexto

El proyecto **ya tiene toda la infraestructura RTC** desacoplada por interfaces:

| Pieza | Archivo | Rol |
|-------|---------|-----|
| `IRtcProvider` | `lib/interfaces/src/IRtcProvider.h` | Contrato: `begin / isRunning / getTime / setTime` |
| `DS1307Provider` | `src/PeripheryManager/DS1307Provider.{h,cpp}` | Driver I2C directo (BCD, 0x68) — RTC de fábrica del Ulanzi |
| `RtcTimeProvider` | `src/RtcTimeProvider.h` | Envuelve el RTC en `ITimeProvider`; prioriza NTP y cae a RTC sin WiFi |
| Cableado | `src/main.cpp:111-122` | Instancia el RTC, lo inyecta en `RtcTimeProvider` |
| Sync NTP→RTC | `src/main.cpp:240-250` | Escribe la hora NTP al RTC una vez tras sincronizar |

Por tanto, **añadir el DS3231 es solo escribir un nuevo driver** que implemente `IRtcProvider` y elegirlo en `main.cpp`. No se toca `RtcTimeProvider`, `AlarmManager`, `NightModePolicy` ni ningún consumidor.

---

## 2. Decisión de diseño

### 2.1 Driver dedicado (no reutilizar DS1307Provider)

El DS3231 y el DS1307 **comparten dirección I2C `0x68`** y los registros de tiempo `0x00–0x06` son casi idénticos, pero difieren en lo importante:

| Aspecto | DS1307 | DS3231 |
|---------|--------|--------|
| Detección de oscilador parado | bit **CH** (reg `0x00`, bit 7) | flag **OSF** (reg de estado `0x0F`, bit 7) |
| Registro de control | `0x07` (salida onda cuadrada) | `0x0E` (control) + `0x0F` (estado) |
| Bit de siglo en el mes | no existe | reg `0x05`, bit 7 (hay que enmascarar al leer) |
| Sensor de temperatura | no | sí (regs `0x11`–`0x12`, ±3 °C) |
| Precisión | deriva con temperatura | TCXO, ±2 ppm (≈1 min/año) |

Si se usara el driver DS1307 con un DS3231, **daría la hora correctamente**, pero:
- `isRunning()` siempre devolvería `true` (el bit CH del DS1307 cae sobre bits de segundos del DS3231, siempre 0) → se pierde la detección de "batería muerta".
- Se ignora el sensor de temperatura interno.

Por eso se implementa un **`DS3231Provider` dedicado** con manejo correcto de OSF.

### 2.2 Selección por flag de compilación (no auto-detección)

Ambos chips viven en `0x68` y **no se distinguen de forma fiable en runtime** (sondear registros altos puede corromper la SRAM del DS1307 o dar falsos positivos). La librería estándar **RTClib tampoco auto-detecta** — obliga a elegir la clase en código. Seguimos esa práctica:

- Flag nuevo: **`-DRTC_DS3231`**.
- **Default = DS1307** (Ulanzi TC001 de fábrica, sin cambios).
- El futuro env `[env:esp32s3]` activa `-DRTC_DS3231`.
- Quien monte un DS3231 en un Ulanzi solo añade el flag a su build (documentado abajo).

### 2.3 Sensor de temperatura — fuera del alcance inicial

El DS3231 trae sensor de temperatura interno, pero exponerlo como fuente de temperatura toca `SensorConfig`, `TempSensorType`, la web UI y MQTT/HA. Se deja como **Fase 2 opcional** (sección 8) para mantener el alcance acotado: timekeeping fiable primero.

---

## 3. Conexiones de hardware

El DS3231 es un módulo I2C de 4 pines (VCC, GND, SDA, SCL). La mayoría de módulos (ZS-042) traen pull-ups de 4.7 kΩ y portapilas para LIR2032/CR2032.

| Pin módulo | Ulanzi TC001 (ESP32) | ESP32-S3 DevKitC-1 | Notas |
|------------|----------------------|--------------------|-------|
| VCC | 3V3 | 3V3 | 3.3 V (NO 5V si se comparte bus con sensores 3V3) |
| GND | GND | GND | Común |
| SDA | GPIO21 | GPIO1 | Mismo bus que SHT3x — comparten líneas |
| SCL | GPIO22 | GPIO2 | Pull-up 4.7 kΩ (suele venir en el módulo) |

> ⚠️ **Batería:** usar **LIR2032 recargable** solo si el módulo trae el circuito de carga (algunos ZS-042 lo traen con una resistencia que conviene quitar si se usa CR2032 no recargable). Con CR2032 no recargable, **quitar el diodo/resistencia de carga** del módulo para no dañar la pila.

> ⚠️ **Pull-ups:** si conviven DS3231 + SHT3x en el mismo bus y ambos módulos traen pull-ups, las resistencias quedan en paralelo (≈2.4 kΩ). Es aceptable a 400 kHz, pero si hay problemas de I2C, quitar los pull-ups de uno de los módulos.

El DS3231 se detecta automáticamente tras `Wire.begin(...)` (ya se llama en `PeripheryManager::setup()` antes del bloque RTC de `main.cpp`).

---

## 4. Archivos a crear / modificar

| Archivo | Acción | Descripción |
|---------|--------|-------------|
| `src/PeripheryManager/DS3231Provider.h` | **Crear** | Header del driver (espejo del DS1307Provider) |
| `src/PeripheryManager/DS3231Provider.cpp` | **Crear** | Implementación I2C con manejo de OSF |
| `src/main.cpp` | Modificar | Selección de driver por flag + tipo de `rtcInstance` |
| `platformio.ini` | Modificar | Documentar el flag `RTC_DS3231` (y activarlo en `esp32s3`) |
| `test/test_native/test_rtc_bcd/` | **Crear** (opcional) | Test nativo de la lógica BCD↔epoch + OSF |
| `lib/interfaces/CLAUDE.md` | Modificar | Añadir `DS3231Provider` a la tabla de implementores de `IRtcProvider` |
| `src/PeripheryManager/CLAUDE.md` | Modificar | Mencionar el driver RTC seleccionable |
| `docs/ESP32-S3-N16R8-PORT.md` | Modificar | DS3231 en BOM + esquemático (ya hecho en este commit) |

---

## 5. Especificación de `DS3231Provider`

### 5.1 Mapa de registros DS3231

| Reg | Contenido | Notas |
|-----|-----------|-------|
| `0x00` | Segundos (BCD 00–59) | bit 7 siempre 0 (≠ CH del DS1307) |
| `0x01` | Minutos (BCD 00–59) | |
| `0x02` | Horas (BCD) | bit 6 = modo 12/24 h (usamos 24 h → enmascarar `0x3F`) |
| `0x03` | Día de la semana (1–7) | |
| `0x04` | Fecha (BCD 01–31) | |
| `0x05` | Mes (BCD 01–12) | **bit 7 = Siglo** → enmascarar `0x1F` al leer |
| `0x06` | Año (BCD 00–99) | offset desde 2000 |
| `0x0E` | Control | bit 7 = **EOSC** (0 = oscilador ON en batería) |
| `0x0F` | Estado | bit 7 = **OSF** (1 = el oscilador se detuvo → hora inválida) |
| `0x11`–`0x12` | Temperatura | Fase 2 (sección 8) |

### 5.2 Diferencias clave respecto al DS1307Provider

1. **`isRunning()`** → leer reg de estado `0x0F` y devolver `(status & 0x80) == 0` (OSF limpio).
   _DS1307 leía el bit CH de `0x00`._
2. **`setTime()`** → tras escribir la hora, **limpiar OSF** (leer `0x0F`, borrar bit 7, reescribir) y asegurar **EOSC = 0** en `0x0E` (oscilador activo en batería). Si no se limpia OSF, `isRunning()` seguiría reportando fallo tras un corte de energía.
3. **`getTime()`** → enmascarar el mes con `0x1F` para descartar el bit de siglo.

El resto (BCD↔dec, lectura/escritura de registros, `mktime`/`gmtime`) es **idéntico al DS1307Provider** — copiar y ajustar.

### 5.3 Esqueleto del header

```cpp
#pragma once

#include "IRtcProvider.h"
#include <Wire.h>

/**
 * @file DS3231Provider.h
 * @brief DS3231 TCXO RTC driver using direct I2C communication.
 *
 * Register-compatible with the DS1307 for time (0x00-0x06) but uses the
 * Oscillator-Stop Flag (OSF, status reg 0x0F bit 7) instead of the DS1307
 * Clock-Halt bit to detect a stopped clock. Address: 0x68.
 */
class DS3231Provider : public IRtcProvider
{
  public:
    DS3231Provider() = default;
    ~DS3231Provider() override = default;
    DS3231Provider(const DS3231Provider&) = delete;
    DS3231Provider& operator=(const DS3231Provider&) = delete;

    bool begin() override;
    bool isRunning() override;     // OSF clear
    time_t getTime() override;     // mask century bit
    bool setTime(time_t epoch) override;  // clear OSF + EOSC=0

  private:
    static constexpr uint8_t kI2cAddress  = 0x68;
    static constexpr uint8_t kRegSeconds  = 0x00;
    static constexpr uint8_t kRegMonth    = 0x05;
    static constexpr uint8_t kRegControl  = 0x0E;
    static constexpr uint8_t kRegStatus   = 0x0F;

    static constexpr uint8_t kOscStopFlag = 0x80;  // OSF, status bit 7
    static constexpr uint8_t kEoscBit     = 0x80;  // EOSC, control bit 7
    static constexpr uint8_t kCenturyBit  = 0x80;  // month bit 7

    bool detected_ = false;

    uint8_t bcdToDec(uint8_t v) const { return (v >> 4) * 10 + (v & 0x0F); }
    uint8_t decToBcd(uint8_t v) const { return ((v / 10) << 4) | (v % 10); }

    bool readRegisters(uint8_t reg, uint8_t *buf, uint8_t count);
    bool writeRegisters(uint8_t reg, const uint8_t *buf, uint8_t count);
};
```

### 5.4 Lógica de OSF en `.cpp` (lo único realmente nuevo)

```cpp
bool DS3231Provider::isRunning()
{
    if (!detected_) return false;
    uint8_t status = 0;
    if (!readRegisters(kRegStatus, &status, 1)) return false;
    return (status & kOscStopFlag) == 0;  // OSF clear ⇒ time valid
}

bool DS3231Provider::setTime(time_t epoch)
{
    if (!detected_) return false;
    struct tm *t = gmtime(&epoch);
    if (!t) return false;

    uint8_t buffer[7];
    buffer[0] = decToBcd(t->tm_sec)  & 0x7F;
    buffer[1] = decToBcd(t->tm_min);
    buffer[2] = decToBcd(t->tm_hour) & 0x3F;   // 24h mode
    buffer[3] = decToBcd(t->tm_wday + 1);
    buffer[4] = decToBcd(t->tm_mday);
    buffer[5] = decToBcd(t->tm_mon + 1);       // century bit stays 0
    buffer[6] = decToBcd(t->tm_year - 100);
    if (!writeRegisters(kRegSeconds, buffer, 7)) return false;

    // Clear OSF so isRunning() reports OK after this set
    uint8_t status = 0;
    if (readRegisters(kRegStatus, &status, 1))
    {
        status &= ~kOscStopFlag;
        writeRegisters(kRegStatus, &status, 1);
    }
    // Ensure oscillator runs on battery (EOSC=0)
    uint8_t control = 0;
    if (readRegisters(kRegControl, &control, 1))
    {
        control &= ~kEoscBit;
        writeRegisters(kRegControl, &control, 1);
    }
    return true;
}
```

`getTime()` y `begin()` son copia del DS1307Provider, salvo enmascarar el mes:
```cpp
t.tm_mon = bcdToDec(buffer[5] & 0x1F) - 1;   // strip century bit
```

---

## 6. Cambios en `main.cpp`

Reemplazar las referencias directas a `DS1307Provider` por un alias seleccionado por flag.

```cpp
// Cerca de los includes (líneas ~37)
#ifdef RTC_DS3231
#include "PeripheryManager/DS3231Provider.h"
using RtcProvider = DS3231Provider;
#else
#include "PeripheryManager/DS1307Provider.h"
using RtcProvider = DS1307Provider;
#endif
```

```cpp
// Línea 47: tipar por interfaz (setTime() está en IRtcProvider)
static IRtcProvider *rtcInstance = nullptr;
```

```cpp
// Líneas 111-122: usar el alias
static RtcProvider rtc;
static RtcTimeProvider rtcTimeProvider;
if (rtc.begin())
{
    rtcTimeProvider.setRtc(&rtc);
    rtcInstance = &rtc;
    if (!rtc.isRunning())
    {
        DEBUG_PRINTLN(F("RTC battery may be dead - clock halted"));
    }
}
```

El bloque de sync NTP→RTC (`main.cpp:240-250`) **no cambia** — usa `rtcInstance->setTime()` vía interfaz.

---

## 7. `platformio.ini`

```ini
; RTC chip selection (default DS1307 = Ulanzi de fábrica):
;   -DRTC_DS3231   selecciona el driver DS3231 (TCXO, recomendado para builds DIY)
; El env esp32s3 lo activa por defecto.

[env:esp32s3]
build_flags =
    ${env:ulanzi.build_flags}
    -DRTC_DS3231
    ; ... resto de flags del port S3
```

Para usar un DS3231 en un Ulanzi: añadir `-DRTC_DS3231` a los `build_flags` del env `ulanzi` (o un env derivado).

---

## 8. Fase 2 (opcional) — Sensor de temperatura del DS3231

El DS3231 expone temperatura en `0x11` (MSB, entero con signo) y `0x12` (bits 7-6 = fracción en pasos de 0.25 °C):

```cpp
float readTemperature()  // ±3 °C, resolución 0.25 °C
{
    uint8_t buf[2];
    if (!readRegisters(0x11, buf, 2)) return NAN;
    return (int8_t)buf[0] + ((buf[1] >> 6) * 0.25f);
}
```

Integrarlo como fuente de temperatura requiere:
- Nuevo valor en `TempSensorType` (`lib/config/src/ConfigTypes.h`).
- Lectura en `PeripheryManager` (cada 10 s, como los demás sensores).
- Web UI (`TimeDateSection` / sensores) + MQTT/HA (`/add-ha-entity`).

> No recomendado de entrada: el sensor del DS3231 es para compensar su propio cristal, no de ambiente — mide la temperatura del chip (suele leer 2-4 °C por encima del aire). Útil solo como respaldo si no hay SHT3x/BME280.

---

## 9. Plan de pruebas

### 9.1 Test nativo (opcional, recomendado)

El `DS1307Provider` **no** tiene test nativo (depende de `Wire`). Para el DS3231, se puede extraer la lógica pura (BCD↔dec, epoch↔buffer, máscara de OSF/siglo) a funciones libres testeables y crear `test/test_native/test_rtc_bcd/`:

- `decToBcd`/`bcdToDec` round-trip para 0–59.
- epoch conocido → buffer de 7 bytes correcto y de vuelta.
- bit de siglo en el mes se enmascara.
- OSF: status con bit 7 → `isRunning()` falso.

Ejecutar con `pio test -e native_test`.

### 9.2 Validación en dispositivo (checklist)

```
[ ] I2C scan detecta 0x68 tras conectar el módulo
[ ] Boot sin WiFi: el reloj arranca con hora del RTC (no 00:00)
[ ] Con WiFi: NTP sincroniza y escribe al RTC (log "NTP time synced to RTC")
[ ] Quitar WiFi y reiniciar: mantiene la hora correcta (RTC)
[ ] Quitar batería del módulo y reiniciar: isRunning()=false, log "battery may be dead"
[ ] Reponer batería + sync NTP: OSF se limpia, isRunning()=true
[ ] AlarmManager dispara alarmas sin WiFi (usa RTC vía RtcTimeProvider)
[ ] NightModePolicy respeta la ventana nocturna sin WiFi
[ ] Deriva tras 24 h sin NTP: < 1 s (vs varios s del DS1307)
```

---

## 10. Documentación a actualizar al implementar

- `lib/interfaces/CLAUDE.md` — fila `IRtcProvider`: implementores `DS1307Provider, DS3231Provider`.
- `src/PeripheryManager/CLAUDE.md` — nota sobre RTC seleccionable por flag.
- `docs/ESP32-S3-N16R8-PORT.md` — BOM + esquemático I2C (✅ hecho en este commit).
- `docs/hardware.md` (+ `docs/en/`, `docs/uk/`) — si se documenta el RTC al usuario final (idioma: 3 idiomas + guía XE1E en español, según convención del repo).
- Changelog: `feat(rtc): add DS3231 driver (TCXO) selectable via RTC_DS3231`.

---

## 11. Resumen de la sesión de implementación (cuando llegue el módulo)

1. Conectar el DS3231 al bus I2C (sección 3) y confirmar `0x68` en un scan.
2. Crear `DS3231Provider.{h,cpp}` (secciones 5.3–5.4).
3. Editar `main.cpp` (sección 6) y `platformio.ini` (sección 7).
4. (Opcional) Test nativo (9.1) → `pio test -e native_test`.
5. Build: `pio run -e ulanzi` (DS1307, sin romper) **y** con `-DRTC_DS3231` (DS3231).
6. Flashear y recorrer el checklist (9.2).
7. Actualizar docs (sección 10) y abrir PR (`/pr`).

**Estimación:** ~1-2 h de codificación + pruebas (el grueso ya está hecho por la capa de interfaces).
