import { useSettings } from "../../../context/SettingsContext";
import { Toggle, Select, Card } from "../../../components/ui";
import { useT } from "../../../i18n";
import styles from "./sections.module.css";

// Bitmask for the Moon app's rotating info text.
const BIT_NAME = 0x01;
const BIT_AGE = 0x02;
const BIT_ILLUM = 0x04;

export function MoonSection() {
  const { settings, instantSave } = useSettings();
  const t = useT();
  if (!settings) return null;
  const s = settings;
  const info = s.MINFO ?? 0;
  const setBit = (bit: number, on: boolean) =>
    instantSave({ MINFO: on ? info | bit : info & ~bit });
  const a = t.apps;

  return (
    <Card title={a.moonTitle}>
      <div class={styles.stack}>
        <Select
          label={a.moonHemisphere}
          value={s.MHEMI ?? 0}
          options={[
            { value: 0, label: a.moonNorth },
            { value: 1, label: a.moonSouth },
          ]}
          onChange={(v) => instantSave({ MHEMI: Number(v) })}
        />
        <p class={styles.hint}>{a.moonInfoHint}</p>
        <Toggle
          label={a.moonShowName}
          checked={(info & BIT_NAME) !== 0}
          onChange={(v) => setBit(BIT_NAME, v)}
        />
        <Toggle
          label={a.moonShowAge}
          checked={(info & BIT_AGE) !== 0}
          onChange={(v) => setBit(BIT_AGE, v)}
        />
        <Toggle
          label={a.moonShowIllum}
          checked={(info & BIT_ILLUM) !== 0}
          onChange={(v) => setBit(BIT_ILLUM, v)}
        />
      </div>
    </Card>
  );
}
