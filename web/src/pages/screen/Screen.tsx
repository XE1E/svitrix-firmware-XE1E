import { useRef, useEffect, useState } from "preact/hooks";
import { getScreen, nextApp, previousApp } from "../../api/client";
import { useT } from "../../i18n";
import { saveBlob } from "../../utils/saveFile";
import styles from "./Screen.module.css";

const COLS = 32;
const ROWS = 8;
const CELL = 33;
const PIX = 29;

// The live mirror polls /api/screen over HTTP. The device's async TCP stack
// leaks sockets under sustained connection churn, so keep the rate modest and
// auto-pause after a while of continuous polling — leaving the tab open for
// hours used to slowly exhaust sockets and freeze the display.
const POLL_INTERVAL_MS = 500; // 2 fps — smooth enough for a status mirror
const HIDDEN_RECHECK_MS = 1000;
const MAX_ACTIVE_POLL_MS = 2 * 60 * 1000; // auto-pause after 2 min of active polling

export function ScreenPage(_props: { path?: string; default?: boolean }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const running = useRef(true);
  const startedAt = useRef(0);
  const [paused, setPaused] = useState(false);
  const [runId, setRunId] = useState(0); // bump to (re)start the poll loop
  const t = useT();

  useEffect(() => {
    running.current = true;
    startedAt.current = Date.now();
    const canvas = canvasRef.current!;
    const ctx = canvas.getContext("2d")!;
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    let timer: ReturnType<typeof setTimeout>;
    async function poll() {
      if (!running.current) return;
      // Don't poll a backgrounded tab — no point and it just churns connections.
      if (document.hidden) {
        timer = setTimeout(poll, HIDDEN_RECHECK_MS);
        return;
      }
      // Cap continuous polling to bound socket churn on the device.
      if (Date.now() - startedAt.current > MAX_ACTIVE_POLL_MS) {
        setPaused(true);
        return;
      }
      try {
        const data = await getScreen();
        for (let i = 0; i < COLS * ROWS; i++) {
          const c = data[i];
          const r = (c & 0xff0000) >> 16;
          const g = (c & 0x00ff00) >> 8;
          const b = c & 0x0000ff;
          const col = i % COLS;
          const row = Math.floor(i / COLS);
          ctx.fillStyle = `rgb(${r},${g},${b})`;
          ctx.fillRect(col * CELL, row * CELL, PIX, PIX);
        }
        timer = setTimeout(poll, POLL_INTERVAL_MS);
      } catch {
        timer = setTimeout(poll, 1000);
      }
    }
    poll();
    return () => {
      running.current = false;
      clearTimeout(timer);
    };
  }, [runId]);

  function resume() {
    setPaused(false);
    setRunId((n) => n + 1); // re-runs the effect → fresh poll loop, timer reset
  }

  function downloadPng() {
    canvasRef.current!.toBlob((blob) => {
      if (blob) saveBlob(blob, "svitrix-screen.png", "image/png");
    }, "image/png");
  }

  return (
    <div>
      <div class={styles.controls}>
        <button onClick={() => previousApp()}>&#9664; {t.screen.prev}</button>
        <button onClick={() => nextApp()}>{t.screen.next} &#9654;</button>
        <button onClick={downloadPng}>{t.screen.downloadPng}</button>
      </div>
      <div class={styles.canvasWrap}>
        <canvas
          ref={canvasRef}
          width={COLS * CELL}
          height={ROWS * CELL}
          class={styles.canvas}
        />
        {paused && (
          <div class={styles.pausedOverlay}>
            <span>{t.screen.paused}</span>
            <button onClick={resume}>{t.screen.resume}</button>
          </div>
        )}
      </div>
    </div>
  );
}
