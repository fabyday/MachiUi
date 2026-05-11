import React, { useEffect, useMemo, useRef, useState } from "react";
import MachiRenderer from "@machi/core/Reconciler/HostConfig";
import { registerStylesheet } from "@machi/core/Reconciler/StyleSheet";

const root = MachiRenderer.createRoot("DemoScene");

const palette = {
  app: "#111827",
  surface: "#f8fafc",
  panel: "#ffffff",
  border: "#d7dee8",
  muted: "#cbd5e1",
  track: "#e2e8f0",
  navy: "#1f2a44",
  cyan: "#10b981",
  blue: "#2563eb",
  amber: "#f59e0b",
  rose: "#e11d48",
};

function clamp(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value));
}

registerStylesheet(`
.screen {
  display: flex;
  flex-direction: row;
  width: 100%;
  height: 100%;
  padding: 24;
  gap: 20;
  background-color: ${palette.app};
}

.sidebar {
  display: flex;
  flex-direction: column;
  width: 210;
  height: 100%;
  padding: 18;
  gap: 14;
  background-color: ${palette.navy};
}

.main {
  display: flex;
  flex-direction: column;
  flex: 1;
  height: 100%;
  padding: 20;
  gap: 18;
  background-color: ${palette.surface};
}

.top-bar {
  display: flex;
  flex-direction: row;
  width: 100%;
  height: 54;
  gap: 12;
  align-items: center;
}

.card-row {
  display: flex;
  flex-direction: row;
  width: 100%;
  height: 132;
  gap: 16;
}

.content {
  display: flex;
  flex-direction: row;
  flex: 1;
  width: 100%;
  gap: 16;
}

.stack {
  display: flex;
  flex-direction: column;
  flex: 1;
  height: 100%;
  gap: 16;
}

.panel {
  display: flex;
  flex-direction: column;
  flex: 1;
  padding: 16;
  gap: 12;
  background-color: ${palette.panel};
}
`);

function Bar({ width, height = 10, color }: { width: number | string; height?: number; color: string }) {
  return <div style={{ width, height, backgroundColor: color }} />;
}

function SidebarItem({ active = false, width, onClick }: { active?: boolean; width: number; onClick?: () => void }) {
  return (
    <div
      onClick={onClick}
      style={{
        display: "flex",
        flexDirection: "row",
        width: "100%",
        height: 36,
        padding: 8,
        gap: 10,
        alignItems: "center",
        backgroundColor: active ? palette.cyan : "#2b3654",
      }}
    >
      <div style={{ width: 18, height: 18, backgroundColor: active ? palette.panel : "#5b6887" }} />
      <Bar width={width} color={active ? palette.panel : "#94a3b8"} />
    </div>
  );
}

function MetricCard({ accent, fill, active, onClick }: { accent: string; fill: number; active?: boolean; onClick?: () => void }) {
  return (
    <div
      onClick={onClick}
      style={{
        display: "flex",
        flexDirection: "column",
        flex: 1,
        height: "100%",
        padding: 16,
        gap: 14,
        backgroundColor: active ? "#ecfdf5" : palette.panel,
      }}
    >
      <div style={{ width: 36, height: 36, backgroundColor: accent }} />
      <span className="text-lg font-semibold text-slate-800" style={{ fontFamily: "Segoe UI" }}>Metric {fill}%</span>
      <Bar width="72%" height={12} color={palette.navy} />
      <div style={{ width: "100%", height: 12, backgroundColor: palette.track }}>
        <Bar width={`${fill}%`} height={12} color={accent} />
      </div>
    </div>
  );
}

function ChartPanel({ accent }: { accent: string }) {
  const bars = [42, 76, 58, 90, 64, 82, 52];

  return (
    <div className="panel">
      <span style={{ color: palette.navy, fontSize: 18, fontFamily: "Segoe UI" }}>Weekly activity</span>
      <Bar width="38%" height={14} color={palette.navy} />
      <div style={{ display: "flex", flexDirection: "row", flex: 1, width: "100%", gap: 10, alignItems: "flex-end" }}>
        {bars.map((height, index) => (
          <div key={index} style={{ flex: 1, height: `${height}%`, backgroundColor: index === 3 ? accent : palette.muted }} />
        ))}
      </div>
    </div>
  );
}

function ActivityPanel() {
  return (
    <div className="panel" style={{ width: 250, flex: 0 }}>
      <span style={{ color: palette.navy, fontSize: 18, fontFamily: "Segoe UI" }}>Recent events</span>
      <Bar width="52%" height={14} color={palette.navy} />
      {[palette.blue, palette.cyan, palette.amber, palette.rose].map((color, index) => (
        <div key={index} style={{ display: "flex", flexDirection: "row", width: "100%", height: 42, gap: 12, alignItems: "center" }}>
          <div style={{ width: 28, height: 28, backgroundColor: color }} />
          <div style={{ display: "flex", flexDirection: "column", flex: 1, gap: 7 }}>
            <Bar width={`${80 - index * 9}%`} height={8} color={palette.navy} />
            <Bar width={`${54 - index * 6}%`} height={7} color={palette.muted} />
          </div>
        </div>
      ))}
    </div>
  );
}

function InteractionPanel({
  accent,
  dragX,
  setDragX,
  lastKey,
  viewport,
}: {
  accent: string;
  dragX: number;
  setDragX: React.Dispatch<React.SetStateAction<number>>;
  lastKey: string;
  viewport: string;
}) {
  const knobRef = useRef<any>(null);
  const [dragging, setDragging] = useState(false);

  useEffect(() => {
    if (!dragging) {
      return;
    }

    const handleMove = (event: any) => {
      const trackRect = knobRef.current?.parentElement?.getBoundingClientRect?.();
      if (trackRect == null) {
        return;
      }

      setDragX(clamp(event.clientX - trackRect.left - 20, 0, 180));
    };
    const handleUp = () => setDragging(false);

    document.addEventListener("pointermove", handleMove);
    document.addEventListener("pointerup", handleUp);
    return () => {
      document.removeEventListener("pointermove", handleMove);
      document.removeEventListener("pointerup", handleUp);
    };
  }, [dragging, setDragX]);

  const dragProps = {
    ref: knobRef,
    onPointerDown: (event: MachiPointerEvent) => {
      knobRef.current?.focus?.();
      setDragging(true);
      event.preventDefault?.();
    },
    onClick: () => {
      setDragX(0);
    },
  } as any;

  return (
    <div className="panel" style={{ flex: 0, height: 134 }}>
      <span className="text-lg font-semibold text-slate-800" style={{ fontFamily: "Segoe UI" }}>
        Input status: key {lastKey} / {viewport}
      </span>
      <div className="w-[220px] h-8 bg-slate-200 overflow-hidden">
        <div
          {...dragProps}
          className="w-10 h-8 animate-pulse"
          style={{ marginLeft: dragX, backgroundColor: accent }}
        />
      </div>
      <div className="flex flex-row w-full h-11 gap-3">
        <Bar width="34%" height={44} color={palette.cyan} />
        <Bar width="28%" height={44} color={palette.blue} />
        <Bar width="22%" height={44} color={palette.amber} />
      </div>
    </div>
  );
}

function App() {
  const accentOptions = [palette.cyan, palette.blue, palette.amber, palette.rose];
  const [accent, setAccent] = useState(palette.cyan);
  const [selectedMetric, setSelectedMetric] = useState(0);
  const [dragX, setDragX] = useState(72);
  const [lastKey, setLastKey] = useState("none");
  const [viewport, setViewport] = useState("initial");
  const metrics = useMemo(
    () => [
      { accent, fill: 72 },
      { accent: palette.blue, fill: 58 },
      { accent: palette.amber, fill: 86 },
    ],
    [accent],
  );
  const screenEvents = {
    onKeyDown: (event: MachiKeyEvent) => {
      setLastKey(String(event.keyCode));
    },
    onWindowResize: (event: MachiWindowEvent) => {
      setViewport(`${event.width}x${event.height}`);
    },
  } as any;

  return (
    <div className="screen" {...screenEvents}>
      <div className="sidebar">
        <div style={{ width: "100%", height: 48, backgroundColor: accent }} />
        <SidebarItem active={selectedMetric === 0} width={96} onClick={() => setSelectedMetric(0)} />
        <SidebarItem active={selectedMetric === 1} width={118} onClick={() => setSelectedMetric(1)} />
        <SidebarItem active={selectedMetric === 2} width={82} onClick={() => setSelectedMetric(2)} />
        <SidebarItem width={106} onClick={() => setAccent(palette.rose)} />
      </div>

      <div className="main">
        <div className="top-bar">
          <span className="text-2xl font-bold text-slate-800" style={{ fontFamily: "Segoe UI", lineHeight: 32 }}>
            Machi UI Dashboard
          </span>
          <div style={{ flex: 1, height: 1, backgroundColor: palette.border }} />
          {accentOptions.map((color) => (
            <div
              key={color}
              className={`w-10 h-10 ${accent === color ? "animate-pulse" : ""}`}
              style={{ backgroundColor: color }}
              onClick={() => setAccent(color)}
            />
          ))}
        </div>

        <div className="card-row">
          {metrics.map((metric, index) => (
            <MetricCard
              key={index}
              accent={metric.accent}
              fill={metric.fill}
              active={selectedMetric === index}
              onClick={() => setSelectedMetric(index)}
            />
          ))}
        </div>

        <div className="content">
          <div className="stack">
            <ChartPanel accent={accent} />
            <InteractionPanel accent={accent} dragX={dragX} setDragX={setDragX} lastKey={lastKey} viewport={viewport} />
          </div>
          <ActivityPanel />
        </div>
      </div>
    </div>
  );
}

root.render(<App />);
