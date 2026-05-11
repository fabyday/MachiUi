---
title: Architecture
description: Main MachiUI engine modules and how they work together.
sidebar:
  order: 1
---

MachiUI is split into native engine services, renderer backends, platform hosts, and the JavaScript runtime layer.

## Core

`Source/Core` contains the service model, scene graph, element primitives, view management, timers, and shared engine types. This layer owns the runtime state that renderers and scripts operate on.

## Elements

`Source/Elements` contains built-in UI element implementations. Text elements also provide layout measurement so Yoga can reserve the correct space before rendering.

## Layout

MachiUI uses Yoga for Flexbox layout. JavaScript props and class names are resolved into element attributes, then the native scene graph computes layout before rendering.

## Renderer

`Source/Renderer` contains render queues and backend interfaces. On Windows, the DirectX 12 backend draws solid UI rectangles and uses DirectWrite/Direct2D interop for text.

## Platform

`Source/MinimalPlatform` contains operating-system hosts. The Windows host owns native windows and forwards pointer, keyboard, resize, focus, and close events into the engine.

## Scripting

`Source/Scripting` embeds QuickJS and exposes `MachiNative` bindings. The React reconciler calls those bindings to create elements, update props, attach event handlers, and query layout geometry.

## JavaScript Runtime

`Source/Javascript` contains the React reconciler, DOM compatibility shim, style utilities, and React DOM compatibility entry points. UI code can use React patterns while still targeting the native scene graph.
