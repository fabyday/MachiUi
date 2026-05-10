import React from "react";
import MachiRenderer from "@machi/core/Reconciler/HostConfig";

const root = MachiRenderer.createRoot("DemoScene");

function App() {
  return (
    <div color="white" style={{ width: 800, height: 600 }}>
      <div color="yellow" style={{ width: 420, height: 260 }}>
        Yellow panel
      </div>
    </div>
  );
}

root.render(<App />);
