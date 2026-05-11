# MachiUi Project Structure Overview

## 1. Source/ (Engine Core & Implementations)
- **Core/**: 엔진의 핵심 로직 및 인터페이스 정의. 특정 플랫폼이나 그래픽 API에 의존하지 않는 순수 가상 클래스(Interface) 위주.
  - `IRenderer.h`: 렌더링 백엔드 추상화 인터페이스.
  - `IService.h`: 엔진의 기능 단위(Service) 베이스.
  - `UiEngine.h`: 엔진의 메인 컨트롤러 및 서비스 생명주기 관리.
  - `ServiceRegistry.h`: 서비스 자동 등록 및 의존성 주입 시스템.

- **Renderer/**: `IRenderer`의 구체적인 구현체들.
  - **Null/**: 하드웨어 가속이 없는 환경(CI/CD, Unit Test)을 위한 No-op 렌더러.
  - **Backend/**: 실제 그래픽 API 구현.
    - **Metal/**: Apple 플랫폼 전용 Metal 기반 렌더러.
    - **DX12/**: Windows 플랫폼 전용 DirectX 12 기반 렌더러. (추가 예정)

- **MinimalPlatform/**: OS 종속적인 최소 기능(Windowing, Input) 구현.
  - **Win/**: Win32 API 기반 창 생성 및 메시지 루프.
  - **Mac/**: AppKit/Cocoa 기반 구현. (추가 예정)

- **Common/**: 프로젝트 전반에서 사용되는 매크로(`macro.h`), 유틸리티, 공통 데이터 구조.

## 2. Test/ (Verification)
- **UnitTest/**: Google Test 및 GMock을 사용한 기능 검증.
  - **Core/Mocks/**: `MockUiEngine.h`와 같이 내부 멤버 접근이 필요한 테스트용 프록시 및 모킹 클래스.

## 3. Architecture Principles
1. **Interface-Based Design**: `Core`는 오직 `IRenderer`, `IWindow` 등 인터페이스만 참조하며, 실제 구현체는 런타임 혹은 빌드 타임에 주입됨.
2. **Service-Oriented**: 엔진의 각 기능은 `IService`를 상속받아 독립적인 모듈로 관리됨.
3. **Backend Separation**: `Renderer/Backend` 내부는 각 API의 고유한 코드(Objective-C++, HLSL 등)를 격리하여 컴파일 의존성을 최소화함.

## 4. Key Implementation Detail
- **Service Injection**: `REGISTER_UI_SERVICE` 매크로를 통해 전역 정적 영역에서 서비스를 등록하고 `UiEngine`에서 로드하는 구조를 취함.
- **Testing Strategy**: 하드웨어가 없는 환경에서도 로직 테스트가 가능하도록 `NullRenderer`를 기본 백엔드로 활용 가능함.