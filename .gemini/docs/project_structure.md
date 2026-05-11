# MachiUi Project Structure Overview

## 1. Source/ (Engine Core & Implementations)
- **Core/**: Contains core engine logic and interface definitions. Focused on pure virtual classes (Interfaces) that remain agnostic of specific platforms or Graphics APIs.
  - `IRenderer.h`: Abstraction interface for rendering backends.
  - `IService.h`: Base class for engine functional units (Services).
  - `IWindow.h` / `IWindowHost.h`: Abstractions for OS windowing.
  - `UiEngine.h`: Main controller managing engine and service lifecycles.
  - `ServiceRegistry.h`: System for automatic service registration and dependency injection.

- **Renderer/**: Concrete implementations of `IRenderer`.
  - **Null/**: A no-op renderer implementation (`NullRenderer.h`) used for headless environments, CI/CD, and Unit Testing.
  - **Backend/**: Actual hardware-accelerated Graphics API implementations.
    - **Metal/**: Metal-based renderer for Apple platforms (Objective-C++).
    - **DX12/**: DirectX 12-based renderer for Windows platforms (Planned).

- **MinimalPlatform/**: Implementation of OS-dependent minimal features such as Windowing and Input handling.
  - **OSX/**: implementation using AppKit/Cocoa (`impl.mm`).
  - **Win/**: implementation using Win32 API (`Win32Window.cpp`).

- **Common/**: Project-wide macros (`macro.h`), utilities, and shared data structures.

## 2. Test/ (Verification)
- **UnitTest/**: Functional verification using Google Test and GMock.
  - **Core/Mocks/**: Mock objects and test proxies (e.g., `MockUiEngine.h`) that allow testing of internal logic through `reinterpret_cast` or friendship bypasses.

## 3. Architecture Principles
1. **Interface-Based Design**: The `Core` only references abstractions like `IRenderer` or `IWindow`. Concrete implementations are injected at runtime or build time.
2. **Service-Oriented**: Each engine feature is managed as an independent module inheriting from `IService`.
3. **Backend Separation**: Code within `Renderer/Backend` isolates API-specific code (Objective-C++, HLSL, etc.) to minimize compilation dependencies.
4. **Mock-First Verification**: Components are designed to be testable without hardware dependencies by using `NullRenderer` or GMock objects.

## 4. Key Implementation Detail
- **Service Injection**: Services are registered in the global static scope via the `REGISTER_UI_SERVICE` macro and loaded by the `UiEngine`.
- **Testing Strategy**: Logic can be verified in GPU-less environments (like Github Actions) by utilizing the `NullRenderer` backend.