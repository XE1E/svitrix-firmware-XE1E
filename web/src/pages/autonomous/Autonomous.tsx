import { useState, useEffect, useRef } from "preact/hooks";
import {
  getAlarms,
  addAlarm,
  updateAlarm,
  deleteAlarm,
  snoozeAlarm,
  dismissAlarm,
} from "../../api/client";
import type { Alarm, AlarmsState } from "../../api/types";
import { useT } from "../../i18n";
import styles from "./Autonomous.module.css";

const DAYS = ["S", "M", "T", "W", "T", "F", "S"]; // index 0 = Sunday (bit 0)

// Minutes from `now` until the alarm's next fire, or -1 if it never will.
// Mirrors firmware AlarmLogic for an at-a-glance "next alarm" display.
function minutesUntil(a: Alarm, now: Date): number {
  if (!a.enabled) return -1;
  const nowMins = now.getHours() * 60 + now.getMinutes();
  const alarmMins = a.hour * 60 + a.minute;
  if (a.oneTime) {
    let d = alarmMins - nowMins;
    if (d < 0) d += 1440;
    return d;
  }
  if (a.days === 0) return -1;
  const wday = now.getDay(); // 0=Sun
  let best = -1;
  for (let off = 0; off < 7; off++) {
    const day = (wday + off) % 7;
    if (!(a.days & (1 << day))) continue;
    let d = off * 1440 + (alarmMins - nowMins);
    if (d < 0) d += 7 * 1440;
    if (best < 0 || d < best) best = d;
  }
  return best;
}

function fmtUntil(mins: number): string {
  const h = Math.floor(mins / 60);
  const m = mins % 60;
  return h > 0 ? `${h}h ${m}m` : `${m}m`;
}

function AlarmsSection() {
  const [state, setState] = useState<AlarmsState | null>(null);
  const [newTime, setNewTime] = useState("07:00");
  const [newLabel, setNewLabel] = useState("");
  const [newOnce, setNewOnce] = useState(false);
  const pollRef = useRef<ReturnType<typeof setInterval>>();
  const t = useT();

  const poll = async () => {
    try {
      setState(await getAlarms());
    } catch {
      // ignore
    }
  };

  useEffect(() => {
    poll();
    pollRef.current = setInterval(poll, 2000);
    return () => clearInterval(pollRef.current);
  }, []);

  const handleAdd = async () => {
    const [h, m] = newTime.split(":").map(Number);
    await addAlarm({
      hour: h,
      minute: m,
      days: newOnce ? 0 : 0x7f,
      enabled: true,
      oneTime: newOnce,
      snoozeMinutes: 5,
      label: newLabel,
      melody: "",
    });
    setNewLabel("");
    setNewOnce(false);
    poll();
  };

  const patch = async (alarm: Alarm, fields: Partial<Alarm>) => {
    await updateAlarm({ ...alarm, ...fields });
    poll();
  };

  const setTime = (alarm: Alarm, value: string) => {
    const [h, m] = value.split(":").map(Number);
    patch(alarm, { hour: h, minute: m });
  };

  const handleDelete = async (id: number) => {
    await deleteAlarm(id);
    poll();
  };

  // Soonest upcoming alarm (computed from browser time, for display only).
  let soonest = -1;
  if (state?.alarms) {
    const now = new Date();
    for (const a of state.alarms) {
      const mu = minutesUntil(a, now);
      if (mu >= 0 && (soonest < 0 || mu < soonest)) soonest = mu;
    }
  }

  const fmtTime = (a: Alarm) =>
    `${a.hour.toString().padStart(2, "0")}:${a.minute.toString().padStart(2, "0")}`;

  return (
    <div class={styles.section}>
      <div class={styles.sectionTitle}>
        {t.alarms.title}
        {soonest >= 0 && (
          <span class={styles.nextAlarm}>
            {t.alarms.nextAlarm}: {fmtUntil(soonest)}
          </span>
        )}
      </div>

      {state?.ringing && (
        <div class={styles.ringingAlert}>
          <div>{t.alarms.ringing}</div>
          <button class={styles.btnPause} onClick={() => { snoozeAlarm(5); poll(); }}>
            {t.alarms.snooze5min}
          </button>
          <button class={styles.btnDanger} onClick={() => { dismissAlarm(); poll(); }}>
            {t.alarms.dismiss}
          </button>
        </div>
      )}

      <div class={styles.alarmList}>
        {state?.alarms.map((alarm) => (
          <div key={alarm.id} class={styles.alarmItem}>
            <div
              class={`${styles.toggle} ${alarm.enabled ? styles.on : ""}`}
              onClick={() => patch(alarm, { enabled: !alarm.enabled })}
            />
            <input
              type="time"
              class={styles.alarmTimeInput}
              value={fmtTime(alarm)}
              onChange={(e) => setTime(alarm, (e.target as HTMLInputElement).value)}
            />

            {alarm.oneTime ? (
              <div class={`${styles.dayBadge} ${styles.active}`}>1×</div>
            ) : (
              <div class={styles.alarmDays}>
                {DAYS.map((d, i) => (
                  <div
                    key={i}
                    class={`${styles.dayBadge} ${alarm.days & (1 << i) ? styles.active : ""}`}
                    onClick={() => patch(alarm, { days: alarm.days ^ (1 << i) })}
                  >
                    {d}
                  </div>
                ))}
              </div>
            )}

            <label class={styles.onceToggle}>
              <input
                type="checkbox"
                checked={alarm.oneTime}
                onChange={(e) =>
                  patch(alarm, {
                    oneTime: (e.target as HTMLInputElement).checked,
                    days: (e.target as HTMLInputElement).checked ? 0 : 0x7f,
                  })
                }
              />
              {t.alarms.once}
            </label>

            <input
              type="text"
              class={styles.alarmLabelInput}
              placeholder={t.alarms.labelPlaceholder}
              value={alarm.label}
              onChange={(e) => patch(alarm, { label: (e.target as HTMLInputElement).value })}
            />
            <input
              type="text"
              class={styles.alarmMelodyInput}
              placeholder={t.alarms.melodyPlaceholder}
              value={alarm.melody}
              onChange={(e) => patch(alarm, { melody: (e.target as HTMLInputElement).value })}
            />
            <label class={styles.snoozeField}>
              {t.alarms.snoozeMin}
              <input
                type="number"
                min={1}
                max={60}
                value={alarm.snoozeMinutes}
                onChange={(e) =>
                  patch(alarm, { snoozeMinutes: Number((e.target as HTMLInputElement).value) || 5 })
                }
              />
            </label>

            <div class={styles.alarmActions}>
              <button class={styles.btnDanger} onClick={() => handleDelete(alarm.id)}>
                X
              </button>
            </div>
          </div>
        ))}

        {(!state?.alarms || state.alarms.length === 0) && (
          <div class={styles.emptyState}>{t.alarms.noAlarms}</div>
        )}
      </div>

      <div class={styles.addAlarm}>
        <input
          type="time"
          value={newTime}
          onChange={(e) => setNewTime((e.target as HTMLInputElement).value)}
        />
        <input
          type="text"
          placeholder={t.alarms.labelPlaceholder}
          value={newLabel}
          onChange={(e) => setNewLabel((e.target as HTMLInputElement).value)}
        />
        <label class={styles.onceToggle}>
          <input
            type="checkbox"
            checked={newOnce}
            onChange={(e) => setNewOnce((e.target as HTMLInputElement).checked)}
          />
          {t.alarms.once}
        </label>
        <button class={styles.btnStart} onClick={handleAdd}>
          {t.alarms.add}
        </button>
      </div>
    </div>
  );
}

export function AutonomousPage(_props: { path?: string }) {
  return (
    <div class={styles.container}>
      <AlarmsSection />
    </div>
  );
}
