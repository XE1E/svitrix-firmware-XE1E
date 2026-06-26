import { useEffect, useRef, useState } from "preact/hooks";
import { getRotation, saveRotation, getEffects } from "../../../api/client";
import type { RotationItem, RotationConfig, EffectInfo } from "../../../api/types";
import { Card, Toggle, Button } from "../../../components/ui";
import { toast } from "../../../components/Toast";
import { useT } from "../../../i18n";
import { useSettings } from "../../../context/SettingsContext";
import styles from "../../settings/sections/sections.module.css";

// Native apps - original first, then weather
const NATIVE_APPS_ORIGINAL = [
  "Time", "Date", "Temperature", "Humidity", "Battery",
];
const NATIVE_APPS_WEATHER = [
  "OutdoorTemp", "OutdoorHum", "Pressure", "AirQuality", "UV",
];
const NATIVE_APPS_EXTRA = [
  "Moon",
];
const ALL_NATIVE_APPS = [...NATIVE_APPS_ORIGINAL, ...NATIVE_APPS_WEATHER, ...NATIVE_APPS_EXTRA];

// Default icon numbers for native apps (from firmware icons.h / NativeApps.cpp)
const DEFAULT_ICONS: Record<string, string> = {
  Time: "-",           // no icon
  Date: "-",           // no icon
  Temperature: "234",  // icon_234 thermometer
  Humidity: "2075",    // icon_2075 water drop
  Battery: "1486",     // icon_1486 battery
  OutdoorTemp: "-",    // dynamic weather icon
  OutdoorHum: "61756", // GIF
  Pressure: "66893",   // GIF
  AirQuality: "73559", // GIF
  UV: "64310",         // GIF
  Moon: "-",           // drawn programmatically (no icon file)
};

function generateId(): string {
  const chars = "0123456789abcdef";
  let id = "";
  for (let i = 0; i < 8; i++) {
    id += chars[Math.floor(Math.random() * 16)];
  }
  return id;
}

const DURATION_MIN = 0;
const DURATION_MAX = 300;
const HOLD_DELAY_MS = 400; // press longer than this → fast repeat
const REPEAT_MS = 110; // repeat interval while held
const FAST_STEP = 10; // step while held

/**
 * −/+ stepper for app duration. Tap = ±1s, press-and-hold = ±10s (repeating).
 * Updates locally for instant feedback and commits once per gesture (on release)
 * to avoid spamming the device with a save per increment.
 */
function DurationStepper({ value, onCommit, defaultDuration }: { value: number; onCommit: (v: number) => void; defaultDuration: number }) {
  const t = useT();
  const [local, setLocal] = useState(value);
  const localRef = useRef(value);
  const interacting = useRef(false);
  const holdTimer = useRef<number | null>(null);
  const repeatTimer = useRef<number | null>(null);

  // Sync from parent when the value changes externally (not mid-gesture).
  useEffect(() => {
    if (!interacting.current) {
      localRef.current = value;
      setLocal(value);
    }
  }, [value]);

  function clamp(v: number): number {
    return Math.max(DURATION_MIN, Math.min(DURATION_MAX, v));
  }

  function apply(delta: number) {
    const next = clamp(localRef.current + delta);
    if (next !== localRef.current) {
      localRef.current = next;
      setLocal(next);
    }
  }

  function start(dir: 1 | -1) {
    interacting.current = true;
    apply(dir); // immediate tap step (±1)
    holdTimer.current = window.setTimeout(() => {
      repeatTimer.current = window.setInterval(() => apply(dir * FAST_STEP), REPEAT_MS);
    }, HOLD_DELAY_MS);
  }

  function stop() {
    if (holdTimer.current !== null) { clearTimeout(holdTimer.current); holdTimer.current = null; }
    if (repeatTimer.current !== null) { clearInterval(repeatTimer.current); repeatTimer.current = null; }
    if (interacting.current) {
      interacting.current = false;
      if (localRef.current !== value) onCommit(localRef.current);
    }
  }

  // Clean up timers if unmounted mid-press.
  useEffect(() => () => {
    if (holdTimer.current !== null) clearTimeout(holdTimer.current);
    if (repeatTimer.current !== null) clearInterval(repeatTimer.current);
  }, []);

  const isDefault = local === 0;
  const display = isDefault ? `${t.apps.default || "Default"} (${defaultDuration}s)` : `${local}s`;

  return (
    <div class={styles.rotationSlider}>
      <label>{t.apps.playlistDuration}</label>
      <div class={styles.stepper}>
        <button
          type="button"
          class={styles.stepperBtn}
          aria-label="−"
          onPointerDown={(e) => { e.preventDefault(); start(-1); }}
          onPointerUp={stop}
          onPointerLeave={stop}
          onPointerCancel={stop}
        >
          −
        </button>
        <span class={`${styles.stepperValue} ${isDefault ? styles.stepperValueDefault : ""}`}>
          {display}
        </span>
        <button
          type="button"
          class={styles.stepperBtn}
          aria-label="+"
          onPointerDown={(e) => { e.preventDefault(); start(1); }}
          onPointerUp={stop}
          onPointerLeave={stop}
          onPointerCancel={stop}
        >
          +
        </button>
      </div>
      <span class={styles.sliderHint}>{t.apps.durationStepHint}</span>
    </div>
  );
}

export function UnifiedRotationSection() {
  const t = useT();
  const { settings, updateSettings, instantSave, weatherConfig, updateWeatherConfig, saveWeatherConfig } = useSettings();
  const [config, setConfig] = useState<RotationConfig | null>(null);
  const [effects, setEffects] = useState<EffectInfo[]>([]);
  const [dragIndex, setDragIndex] = useState<number | null>(null);
  const [overIndex, setOverIndex] = useState<number | null>(null);
  const [expandedId, setExpandedId] = useState<string | null>(null);
  const [showModal, setShowModal] = useState(false);
  const [modalTab, setModalTab] = useState<"app" | "effect">("app");

  useEffect(() => {
    let cancelled = false;

    // Load the rotation config. The firmware always populates defaults at boot
    // (migrateToRotationConfig), so an empty list almost always means we polled
    // while the device was still starting up — not that the config is really
    // empty. Retry a few times before falling back, and NEVER auto-save
    // defaults: doing so would overwrite a valid config (incl. per-item colors)
    // that simply hadn't loaded yet.
    async function loadRotation(attempt = 0) {
      try {
        const data = await getRotation();
        if (cancelled) return;
        if (data.items && data.items.length > 0) {
          setConfig(data);
          return;
        }
        if (attempt < 3) {
          setTimeout(() => { if (!cancelled) loadRotation(attempt + 1); }, 800);
          return;
        }
        // Still empty after retries: show defaults locally for editing only.
        // Persistence happens on the first explicit user edit (commitItems).
        const defaultItems: RotationItem[] = NATIVE_APPS_ORIGINAL.map(name => ({
          id: generateId(),
          type: "app",
          name,
          enabled: true,
          duration: 0,
          color: 0,
          icon: "",
        }));
        setConfig({ items: defaultItems });
      } catch {
        // Network error: leave config as-is (null = loading); a later reload
        // picks up the real data. Never fabricate or save anything here.
      }
    }

    loadRotation();
    getEffects().then(setEffects).catch(() => {});
    return () => { cancelled = true; };
  }, []);

  function label(name: string): string {
    const a = t.apps;
    switch (name) {
      case "Time": return a.time;
      case "Date": return a.date;
      case "Temperature": return a.temperature;
      case "Humidity": return a.humidity;
      case "Battery": return a.battery;
      case "OutdoorTemp": return a.outdoorTemp;
      case "OutdoorHum": return a.outdoorHum;
      case "Pressure": return a.pressure;
      case "AirQuality": return a.airQuality;
      case "UV": return a.uvIndex;
      case "Moon": return a.moon;
      default: return name;
    }
  }

  async function commitItems(items: RotationItem[]) {
    if (!config) return;
    const updated = { ...config, items };
    setConfig(updated);
    try {
      await saveRotation({ items });
      toast(t.saved || "Saved");
    } catch {
      toast(t.errorSaving);
    }
  }

  function handleDrop(target: number) {
    const from = dragIndex;
    setDragIndex(null);
    setOverIndex(null);
    if (!config || from === null || from === target) return;
    const next = config.items.slice();
    const [moved] = next.splice(from, 1);
    next.splice(target, 0, moved);
    commitItems(next);
  }

  function toggleItem(id: string, enabled: boolean) {
    if (!config) return;
    const next = config.items.map(item =>
      item.id === id ? { ...item, enabled } : item
    );
    commitItems(next);
  }

  function updateItem(id: string, patch: Partial<RotationItem>) {
    if (!config) return;
    const next = config.items.map(item =>
      item.id === id ? { ...item, ...patch } : item
    );
    commitItems(next);
  }

  function deleteItem(id: string) {
    if (!config) return;
    const next = config.items.filter(item => item.id !== id);
    commitItems(next);
  }

  function addItem(type: "app" | "effect", name: string) {
    if (!config) return;
    const item: RotationItem = {
      id: generateId(),
      type,
      name,
      enabled: true,
      duration: 0,
      color: 0,
      icon: "",
    };
    const next = [...config.items, item];
    commitItems(next);
    setShowModal(false);
  }

  function toggleExpand(id: string) {
    setExpandedId(expandedId === id ? null : id);
  }

  function isNative(name: string): boolean {
    return ALL_NATIVE_APPS.includes(name);
  }

  async function toggleCelsius(celsius: boolean) {
    await updateSettings({ CEL: celsius });
  }

  if (!config) return null;

  return (
    <>
      <Card title={t.apps.title}>
        <div class={styles.stack}>
          {config.items.length === 0 ? (
            <p class={styles.hint}>{t.apps.playlistEmpty}</p>
          ) : (
            <div class={styles.orderList}>
              {config.items.map((item, i) => (
                <div key={item.id}>
                  <div
                    class={`${styles.orderItem} ${styles.playlistItem} ${dragIndex === i ? styles.orderItemDragging : ""} ${overIndex === i ? styles.orderItemOver : ""} ${!item.enabled ? styles.orderItemDisabled : ""}`}
                    style={{ cursor: "pointer" }}
                    onDragOver={(e) => { e.preventDefault(); setOverIndex(i); }}
                    onDrop={(e) => { e.preventDefault(); handleDrop(i); }}
                    onClick={() => toggleExpand(item.id)}
                  >
                    {/* Left handle: drag-and-drop reorder only (doesn't toggle expand). */}
                    <span
                      class={styles.dragHandle}
                      draggable
                      onDragStart={() => setDragIndex(i)}
                      onDragEnd={() => { setDragIndex(null); setOverIndex(null); }}
                      onClick={(e) => e.stopPropagation()}
                      style={{ cursor: "grab" }}
                    >
                      ☰
                    </span>
                    <span onClick={(e) => e.stopPropagation()}>
                      <Toggle
                        checked={item.enabled}
                        onChange={(v) => toggleItem(item.id, v)}
                        compact
                      />
                    </span>
                    <span class={styles.orderName} style={{ opacity: item.enabled ? 1 : 0.5 }}>
                      {item.type === "effect" ? `✨ ${item.name}` : label(item.name)}
                    </span>
                    {item.type === "effect" && (
                      <span class={styles.playlistEffectBadge}>{t.apps.playlistEffectBadge}</span>
                    )}
                    {!isNative(item.name) && item.type === "app" && (
                      <span class={styles.orderCustomBadge}>custom</span>
                    )}
                    {item.duration > 0 && (
                      <span class={styles.playlistDuration}>
                        {item.duration}s
                      </span>
                    )}
                    <button
                      class={styles.expandBtn}
                      onClick={(e) => { e.stopPropagation(); toggleExpand(item.id); }}
                      title="Settings"
                    >
                      {expandedId === item.id ? "▲" : "▼"}
                    </button>
                    <button
                      class={styles.playlistDeleteBtn}
                      onClick={(e) => { e.stopPropagation(); deleteItem(item.id); }}
                      title={t.apps.playlistDelete}
                    >
                      ✕
                    </button>
                  </div>

                  {expandedId === item.id && (
                    <div class={styles.rotationExpanded}>
                      <div class={styles.rotationRow}>
                        <DurationStepper
                          value={item.duration}
                          onCommit={(v) => updateItem(item.id, { duration: v })}
                          defaultDuration={settings?.ATIME ?? 7}
                        />
                        {/* Effects don't use a per-item color or icon — only duration applies. */}
                        {item.type === "app" && (
                          <div class={styles.rotationField}>
                            <label>Color:</label>
                            <input
                              type="color"
                              value={item.color ? "#" + item.color.toString(16).padStart(6, "0") : "#ffffff"}
                              onInput={(e) => updateItem(item.id, { color: parseInt((e.target as HTMLInputElement).value.replace("#", ""), 16) })}
                            />
                            {item.color === 0 && <span class={styles.hint}>(default)</span>}
                          </div>
                        )}
                        {item.type === "app" && item.name !== "Time" && item.name !== "Date" && item.name !== "Moon" && (
                          <div class={styles.rotationField}>
                            <label>{t.apps.icon || "Icono"}:</label>
                            <input
                              type="text"
                              class={styles.iconInput}
                              value={item.icon}
                              onChange={(e) => updateItem(item.id, { icon: (e.target as HTMLInputElement).value })}
                              placeholder={DEFAULT_ICONS[item.name] || "-"}
                            />
                            {!item.icon && DEFAULT_ICONS[item.name] && (
                              <span class={styles.hint}>(#{DEFAULT_ICONS[item.name]})</span>
                            )}
                          </div>
                        )}
                        {(item.name === "Temperature" || item.name === "OutdoorTemp") && settings && (
                          <div class={styles.rotationField}>
                            <Toggle
                              label={t.apps.celsius}
                              checked={settings.CEL}
                              onChange={toggleCelsius}
                            />
                          </div>
                        )}
                        {item.name === "Temperature" && settings && (
                          <div class={styles.rotationField}>
                            <label>{t.apps.offset}:</label>
                            <input
                              type="number"
                              class={styles.offsetInput}
                              value={settings.TOFF ?? -9}
                              min={-15}
                              max={5}
                              onChange={(e) => updateSettings({ TOFF: parseInt((e.target as HTMLInputElement).value) || 0 })}
                            />
                            <span class={styles.hint}>°</span>
                          </div>
                        )}
                        {item.name === "AirQuality" && weatherConfig && (
                          <div class={styles.rotationField}>
                            <Toggle
                              label={t.apps.autoColor || "Auto color"}
                              checked={weatherConfig.aqiAutoColor}
                              onChange={(v) => { updateWeatherConfig({ aqiAutoColor: v }); saveWeatherConfig(); }}
                            />
                            <Toggle
                              label={t.apps.aqiShowComponents}
                              checked={weatherConfig.aqiShowComponents}
                              onChange={(v) => { updateWeatherConfig({ aqiShowComponents: v }); saveWeatherConfig(); }}
                            />
                          </div>
                        )}
                        {item.name === "UV" && weatherConfig && (
                          <div class={styles.rotationField}>
                            <Toggle
                              label={t.apps.autoColor || "Auto color"}
                              checked={weatherConfig.uvAutoColor}
                              onChange={(v) => { updateWeatherConfig({ uvAutoColor: v }); saveWeatherConfig(); }}
                            />
                          </div>
                        )}
                        {item.name === "Moon" && settings && (
                          <>
                            <div class={styles.rotationField}>
                              <label>{t.apps.moonHemisphere}:</label>
                              <select
                                value={settings.MHEMI ?? 0}
                                onChange={(e) => instantSave({ MHEMI: parseInt((e.target as HTMLSelectElement).value) || 0 })}
                              >
                                <option value={0}>{t.apps.moonNorth}</option>
                                <option value={1}>{t.apps.moonSouth}</option>
                              </select>
                            </div>
                            <div class={styles.rotationField}>
                              <Toggle
                                label={t.apps.moonShowName}
                                checked={((settings.MINFO ?? 0) & 1) !== 0}
                                onChange={(v) => instantSave({ MINFO: v ? (settings.MINFO ?? 0) | 1 : (settings.MINFO ?? 0) & ~1 })}
                              />
                            </div>
                            <div class={styles.rotationField}>
                              <Toggle
                                label={t.apps.moonShowAge}
                                checked={((settings.MINFO ?? 0) & 2) !== 0}
                                onChange={(v) => instantSave({ MINFO: v ? (settings.MINFO ?? 0) | 2 : (settings.MINFO ?? 0) & ~2 })}
                              />
                            </div>
                            <div class={styles.rotationField}>
                              <Toggle
                                label={t.apps.moonShowIllum}
                                checked={((settings.MINFO ?? 0) & 4) !== 0}
                                onChange={(v) => instantSave({ MINFO: v ? (settings.MINFO ?? 0) | 4 : (settings.MINFO ?? 0) & ~4 })}
                              />
                            </div>
                          </>
                        )}
                      </div>
                    </div>
                  )}
                </div>
              ))}
            </div>
          )}

          <div class={styles.buttonRow}>
            <Button onClick={() => setShowModal(true)}>
              ➕ {t.apps.playlistAdd}
            </Button>
            <Button onClick={() => {
              const defaultItems: RotationItem[] = NATIVE_APPS_ORIGINAL.map(name => ({
                id: generateId(),
                type: "app" as const,
                name,
                enabled: true,
                duration: 0,
                color: 0,
                icon: "",
              }));
              commitItems(defaultItems);
            }}>
              🔄 {t.apps.resetDefaults || "Reset"}
            </Button>
          </div>
          <p class={styles.hintMt}>{t.apps.playlistHint}</p>
        </div>
      </Card>

      {showModal && (
        <div class={styles.modalOverlay} onClick={() => setShowModal(false)}>
          <div class={styles.modal} onClick={(e) => e.stopPropagation()}>
            <div class={styles.modalHeader}>
              <h3>{t.apps.playlistAddTitle}</h3>
              <button class={styles.modalClose} onClick={() => setShowModal(false)}>✕</button>
            </div>
            <div class={styles.modalBody}>
              <div class={styles.tabRow}>
                <button
                  class={`${styles.tab} ${modalTab === "app" ? styles.tabActive : ""}`}
                  onClick={() => setModalTab("app")}
                >
                  {t.apps.playlistTypeApp}
                </button>
                <button
                  class={`${styles.tab} ${modalTab === "effect" ? styles.tabActive : ""}`}
                  onClick={() => setModalTab("effect")}
                >
                  {t.apps.playlistTypeEffect}
                </button>
              </div>

              <div class={styles.itemList}>
                {modalTab === "app" ? (
                  <>
                    {NATIVE_APPS_ORIGINAL.map((name) => (
                      <button
                        key={name}
                        class={styles.itemOption}
                        onClick={() => addItem("app", name)}
                      >
                        {label(name)}
                      </button>
                    ))}
                    <div class={styles.itemSeparator}>{t.apps.weatherApps}</div>
                    {NATIVE_APPS_WEATHER.map((name) => (
                      <button
                        key={name}
                        class={styles.itemOption}
                        onClick={() => addItem("app", name)}
                      >
                        {label(name)}
                      </button>
                    ))}
                    {NATIVE_APPS_EXTRA.map((name) => (
                      <button
                        key={name}
                        class={styles.itemOption}
                        onClick={() => addItem("app", name)}
                      >
                        {label(name)}
                      </button>
                    ))}
                  </>
                ) : (
                  effects.map((effect, index) => (
                    <button
                      key={effect.name}
                      class={styles.itemOption}
                      onClick={() => addItem("effect", effect.name)}
                    >
                      {index + 1}. ✨ {effect.name}
                    </button>
                  ))
                )}
              </div>
            </div>
          </div>
        </div>
      )}
    </>
  );
}
