# Gaps de documentación — auditoría código vs. manuales

> Documento interno de referencia. Resultado de una auditoría cruzada (código
> vs. `docs/`, READMEs y guía XE1E) realizada el **2026-06-24**.
> No publicado en el sitio VitePress.

## Resumen

No existen "módulos secretos" ni funciones de peso sin describir. La cobertura
de la documentación es **sólida**. Los gaps reales son pequeños y acotados:
5 puntos de documentación + 1 limpieza de código muerto + algunos detalles menores.

> **Estado (rama `chore/doc-gaps-mqtt-cleanup-battery`):** los 5 gaps de
> documentación y la limpieza MQTT ya se resolvieron (ver tablas abajo). Quedan
> pendientes solo los "detalles menores" de baja prioridad. La gran mayoría de
> lo que los agentes marcaron resultó ya documentado (sección final).

La auditoría se hizo con varios agentes en paralelo; **muchos de sus hallazgos
fueron falsos positivos** (reportaron como "no documentado" cosas que sí lo
están). Cada punto de abajo fue verificado a mano.

---

## Gaps de documentación (verificados)

| # | Gap | Qué falta | Código | Dónde documentarlo |
|---|-----|-----------|--------|--------------------|
| 1 | **Transiciones 11–13** | Existen 14 transiciones (0–13): `SLIDE_UP` (11), `SLIDE_LEFT` (12), `SLIDE_RIGHT` (13). `api.md` dice que `TEFF` es `0–10`. | `src/MatrixDisplayUi/MatrixDisplayUi.h:82-84` · `docs/api.md:555` | `docs/api.md` (rango de `TEFF`) + lista de transiciones de la guía |
| 2 | **Combo oculto en modo AP** | Mantener pulsado **Select** en modo AP cicla el layout de matriz (0→1→2) y reinicia. Única vía documentada: key `matrix` de `dev.json`. | `src/PeripheryManager/PeripheryManager.cpp:171-177` | `docs/onscreen.md` y/o `docs/hardware.md` (troubleshooting de matriz) |
| 3 | **Endpoints WiFi** | `GET/POST /api/wifi` (listar/configurar redes) y `/api/eraseWifi` (borrar solo WiFi) no están en `api.md`. | `src/ServerManager/ServerManager.cpp:196,548,560` | `docs/api.md` |
| 4 | **Endpoints de backup** | `GET /api/settings/export` y `POST /api/settings/import`. `backup.md` cubre el concepto pero no los endpoints REST. | `src/ServerManager/ServerManager.cpp:245-248` | `docs/api.md` (referenciar desde `backup.md`) |
| 5 | **Keys de `/api/settings` sin tabla** | `GAMMA`, `OVERLAY`, `WAPI_UVAUTO`, `WAPI_AQAUTO` se aceptan vía API pero no están en la tabla de settings de `api.md`. (`MINFO`/`MHEMI` ya quedaron en la guía con la app Luna.) | `src/DisplayManager/DisplayManager_Settings.cpp` | `docs/api.md` (tabla de settings) |

---

## Limpieza de código (recomendada)

| Item | Detalle | Código |
|------|---------|--------|
| **Topics MQTT muertos** | `/brightness`, `/timeformat`, `/dateformat` están en `getSubscriptionTopics()` pero `routeTopic()` no los maneja → caen en `CMD_UNKNOWN`. El reloj se suscribe a 3 topics que no hacen nada. | `lib/services/src/MessageRouter.cpp:63,78,79` (quitar de la lista). Test: `test/test_native/test_message_router/` |

Riesgo bajo. Cambio mínimo. Recomendado para reducir confusión y ahorrar suscripciones.

---

## Detalles menores (baja prioridad)

- **Campos raw de `/api/stats`** sin documentar: `bat_raw`, `ldr_raw`, `ram_total`,
  `messages`, `indicator1/2/3`, `matrix`, `type` (diagnóstico). — `lib/services/src/StatsBuilder.cpp`
- **Campo `iconOffset`** de custom apps: aceptado pero no documentado.
  `bounce`/`bounceDir` definidos pero no cableados al payload. — `src/AppContent.h:26`, `src/Apps/Apps.h:13,19`
- **Reset de fábrica (GPIO13, 5s):** documentado en la tabla de hardware y la guía,
  pero ausente en `docs/onscreen.md` (menú on-screen).

---

## Verificado que SÍ está documentado (no re-marcar en futuras auditorías)

Los agentes los reportaron como gaps, pero están correctamente documentados:

- **moodlight / "Luz ambiental"** → `docs/api.md` (topic `/moodlight` + `POST /api/moodlight`)
- **Artnet / DMX** → `docs/effects.md` (gated tras flag de build `-DARTNET`, off por defecto)
- **66 entidades Home Assistant** → `docs/home-assistant.md` (por categoría, incl. `reset_reason`)
- **30 keys de `dev.json`** → `docs/dev.md` (cobertura completa)
- **Easter egg de Año Nuevo** (`new_year`) → `docs/dev.md`
- **Backup/restore** (concepto) → `docs/backup.md`
- **20 efectos · 7 TMODEs · 7 overlays · 8 comandos de dibujo** → `docs/apps.md` / `docs/effects.md` / `docs/api.md`
