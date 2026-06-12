#include "FreezeDebug.h"

#ifdef FREEZE_DEBUG

volatile const char *g_fzLoopStage = "boot";
volatile const char *g_fzRenderStage = "-";
volatile const char *g_fzApp = "-";
volatile int g_fzCondCode = 0;
volatile float g_fzOutdoorTemp = 0.0f;
volatile bool g_fzWeatherValid = false;
volatile uint32_t g_fzLoopCount = 0;

// Prints a heartbeat once per second from core 0, independent of the main loop.
// When the loop wedges, loop# stops advancing and `stuck` counts the seconds since
// the last loop iteration — the stage/render/app fields then point at the hang site.
static void freezeMonitorTask(void *)
{
    uint32_t lastLoopCount = 0;
    uint32_t stuckSeconds = 0;
    for (;;)
    {
        uint32_t lc = g_fzLoopCount;
        if (lc != lastLoopCount)
        {
            lastLoopCount = lc;
            stuckSeconds = 0;
        }
        else
        {
            stuckSeconds++;
        }

        Serial.printf(
            "[FZ %lu] loop#=%lu stuck=%lus | step=%s render=%s app=%s | "
            "cond=%d temp=%.1f valid=%d | heap=%u%s\n",
            (unsigned long)millis(),
            (unsigned long)lc,
            (unsigned long)stuckSeconds,
            g_fzLoopStage, g_fzRenderStage, g_fzApp,
            g_fzCondCode, (double)g_fzOutdoorTemp, (int)g_fzWeatherValid,
            (unsigned)ESP.getFreeHeap(),
            (stuckSeconds >= 3) ? "  <<<<< FROZEN HERE" : "");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void startFreezeMonitor()
{
    xTaskCreatePinnedToCore(freezeMonitorTask, "fzmon", 4096, nullptr, 1, nullptr, 0);
    Serial.println(F("[FZ] freeze monitor started (core 0, 1s heartbeat)"));
}

#endif // FREEZE_DEBUG
