---
title: Runtime Capabilities
description: Optional JavaScript host features exposed by MachiUI.
sidebar:
  order: 2
---

MachiUI separates UI compatibility from optional host capabilities. Browser-like UI APIs should exist when they help React libraries run, but sensitive or platform-dependent features should remain explicit runtime decisions.

## Network

Network access is optional and disabled by default. JavaScript can still reference `fetch`, `Request`, `Response`, and `Headers`, but `fetch()` rejects with a clear error until the host enables the capability.

Enable network access from the native host runtime:

```cpp
auto* scriptManager = provider.getService<ScriptManager>();
scriptManager->setNetworkEnabled(true);
```

Check the capability from JavaScript:

```ts
if (MachiRuntime.hasCapability("network")) {
  const response = await fetch("https://example.com/data.json");
  const data = await response.json();
}
```

This keeps UI libraries usable while avoiding accidental network support in environments that do not want it.

## Input

Pointer, mouse, keyboard, and window events are part of the UI runtime. React-style handlers such as `onClick`, `onPointerDown`, `onPointerMove`, `onKeyDown`, and `onWindowResize` are routed through the native event bridge.

## DOM Compatibility

MachiUI provides a DOM-like compatibility layer for UI libraries that expect `window`, `document`, basic element nodes, event listeners, geometry queries, observers, timers, and animation frames.

This layer is not a full browser DOM. It exists to support UI behavior that maps cleanly to native rendering.
