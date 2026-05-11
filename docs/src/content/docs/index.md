---
title: MachiUI
description: Native C++ UI engine with a React-style JavaScript runtime.
sidebar:
  order: 1
---

MachiUI is a C++17 UI engine that renders declarative interfaces through a native graphics backend instead of embedding a browser engine. It combines a React-style JavaScript runtime, Yoga-based layout, and a platform renderer such as DirectX 12.

## What MachiUI Provides

- A native scene graph for UI elements.
- A React reconciler bridge running on QuickJS.
- Flexbox layout powered by Yoga.
- A DirectX 12 renderer for Windows.
- Input dispatch for pointer, keyboard, and window events.
- Optional runtime capabilities such as network access.

## Documentation Map

- [Installation](getting-started/installation/) explains how to build the asset bundle and native engine.
- [Architecture](engine/architecture/) describes the main engine modules.
- [Runtime Capabilities](engine/runtime-capabilities/) documents optional JavaScript host features.
- [C++ API Documentation](reference/cpp-api/) explains how Doxygen should fit into this site.
