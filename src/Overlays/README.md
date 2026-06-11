# Overlays

Overlay rendering functions drawn on top of the LED matrix during app display.
Provides the status overlay (WiFi/MQTT connection indicators), menu overlay,
and notification overlay with queue management.

## Key Files

| File | Purpose |
|------|---------|
| `Overlays.h` | `Notification` struct, overlay function declarations, notification queue |
| `Overlays.cpp` | `StatusOverlay`, `MenuOverlay`, `NotifyOverlay` implementations |

## Overlay Functions

| Function | Purpose |
|----------|---------|
| `StatusOverlay` | Corner status pixels: WiFi (red, top-left 0,0) and MQTT/HA (yellow, bottom-left 0,7) |
| `MenuOverlay` | Renders the on-device settings menu when active |
| `NotifyOverlay` | Renders notifications from the `notifications` deque with icon, text, effects |

### StatusOverlay states

Hard square-wave blink via `TextEffect(color, 0, period)` (color first half of
the period, black the second).

**WiFi — top-left (0,0), red:**

| State | Pattern | Condition |
|-------|---------|-----------|
| Connected & healthy | off | — |
| WiFi up but fetches failing (likely no internet) | slow blink (1100 ms) | `!DataFetcher.fetchHealthy()` |
| WiFi dropped, reconnecting | fast blink (300 ms) | `!WiFi.isConnected()` |
| AP mode (awaiting config) | solid | `systemConfig.apMode` |

Priority: AP mode > WiFi down > fetch unhealthy. `fetchHealthy()` reflects the
last *network-layer* fetch outcome (a bad jsonPath/parse error does not trip it).

**MQTT/HA — bottom-left (0,7), yellow:**

| State | Pattern | Condition |
|-------|---------|-----------|
| Connected, or MQTT not configured | off | — |
| Configured but disconnected from broker | slow blink (1100 ms) | `!mqttConfig.host.isEmpty() && !MQTTManager.isConnected()` |

## Notification Struct

Extends `AppContentBase` with notification-specific fields: `duration`, `repeat`,
`hold`, `wakeup`, `sound`, `rtttl`, and scroll starting from the right edge.
