#pragma once

/**
 * @file FreezeDebug.h
 * @brief Temporary instrumentation to locate the "outdoor temp freeze".
 *
 * The main Arduino loop appears to hang inside DisplayManager.tick() while the
 * OutdoorTemp app is current (web stays responsive, alarms stop firing, display
 * frozen). To pinpoint WHERE, we keep a set of volatile checkpoints updated as
 * the loop/render advances, plus a free counter bumped every loop().
 *
 * A separate monitor task (core 0) prints these every second. Because it runs on
 * its own task, it KEEPS PRINTING even when the loop is wedged — so the last line
 * shows the exact stage where execution stopped, and `loop#` stops incrementing.
 *
 * All of this is compiled out unless -DFREEZE_DEBUG is set (env: ulanzi_freeze).
 * Zero cost in the release build. Setting a checkpoint is a single pointer write
 * (atomic on ESP32), so the instrumentation cannot itself perturb timing.
 */

#ifdef FREEZE_DEBUG

#include <Arduino.h>

extern volatile const char *g_fzLoopStage;   // which loop() step (main.cpp)
extern volatile const char *g_fzRenderStage; // sub-stage inside MatrixDisplayUi::tick()
extern volatile const char *g_fzApp;         // current native app being rendered
extern volatile int g_fzCondCode;            // weatherData.conditionCode snapshot
extern volatile float g_fzOutdoorTemp;       // weatherData.outdoorTemp snapshot
extern volatile bool g_fzWeatherValid;       // weatherData.valid snapshot
extern volatile uint32_t g_fzLoopCount;      // ++ every loop() — proves loop is alive

#define FZ_LOOP(s) (g_fzLoopStage = (s))
#define FZ_RENDER(s) (g_fzRenderStage = (s))
#define FZ_APP(s) (g_fzApp = (s))
#define FZ_LOOPTICK() (++g_fzLoopCount)
#define FZ_WEATHER(c, t, v)        \
    do {                           \
        g_fzCondCode = (c);        \
        g_fzOutdoorTemp = (t);     \
        g_fzWeatherValid = (v);    \
    } while (0)

void startFreezeMonitor();

#else // FREEZE_DEBUG not defined — compile everything out

#define FZ_LOOP(s)
#define FZ_RENDER(s)
#define FZ_APP(s)
#define FZ_LOOPTICK()
#define FZ_WEATHER(c, t, v)
static inline void startFreezeMonitor() {}

#endif // FREEZE_DEBUG
