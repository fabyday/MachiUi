// JavaScript/src/App.tsx
import React from "react";
import { Machi } from "../renderer/machi-ui";

const App = () => (
  <div style={{ backgroundColor: "red", width: 100, height: 100 }}>
    <div style={{ backgroundColor: "blue", width: 50, height: 50 }} />
  </div>
);

Machi.render(<App />, globalThis.root_handle);
