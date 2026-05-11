# 🚀 Machi-UI

**네이티브 그래픽 API 기반 엔진에서 React의 선언적 모델로 UI를 구성할 수 있도록 한 라이브러리입니다.**

<p align="left">
  <a href="https://fabyday.github.io/MachiUi/">
    <img src="https://img.shields.io/badge/Docs-Open%20MachiUI%20Site-ff5d01?style=for-the-badge&logo=astro&logoColor=white" alt="Open MachiUI Documentation Site" />
  </a>
</p>





## 🌟 Key Features

* **Native Performance**: Electron이나 브라우저 엔진 없이 그래픽스(DX12, OpenGL, etc) 파이프라인에서 직접 렌더링하여 최소한의 오버헤드를 보장합니다.
* **Declarative UI**: React의 Component, Hooks, State 관리 시스템을 그대로 사용하여 복잡한 UI를 쉽게 구축합니다.
* **Flexbox Layout**: Yoga Engine 등을 내장하여 웹 개발자에게 익숙한 레이아웃 방식을 지원합니다.

---

## 🛠 Architecture

이 프로젝트는 React에서 생성된 가상 DOM 노드를 C++의 네이티브 렌더러로 전달하는 브리지 구조를 가집니다.



1.  **Reconciler**: React의 변경 사항을 감지하여 UI 트리 업데이트를 관리합니다.
2.  **Layout Engine**: 계산된 스타일 정보를 바탕으로 각 요소의 좌표를 산출합니다.
3.  **DX12 Backend**: Vertex/Index Buffer와 Shader를 통해 UI를 최종 드로우 콜로 변환합니다.

---

## 🚀 Getting Started

### Prerequisites
* **OS**: Windows 10/11 (DirectX 12 support)
* **Compiler**: MSVC (Visual Studio 2022+)
* **Build Tool**: CMake 3.10+, Git
* **JavaScript Runtime**: Node.js, pnpm
* **Engine**: QuickJS(Runtime) & Yoga Engine(Layout Engine)

### 1. Assets 설치 및 빌드

`Assets/TestUI`는 MachiUi에서 실행할 테스트 UI 번들을 만드는 별도 pnpm 프로젝트입니다.

```powershell
cd Assets/TestUI
pnpm install
pnpm build
```

개발 중 파일 변경을 감시하면서 다시 빌드하려면 다음 명령을 사용합니다.

```powershell
cd Assets/TestUI
pnpm dev
```

기본 `Assets` 패키지를 직접 실행해야 하는 경우에는 다음 명령을 사용할 수 있습니다.

```powershell
cd Assets
pnpm install
pnpm start
```

### 2. MachiUi 설치 및 빌드

처음 저장소를 받은 뒤에는 서브모듈을 먼저 내려받습니다. `ThirdParty/yoga`, `ThirdParty/quickjs`, `ThirdParty/spdlog`가 필요합니다.

```powershell
git submodule update --init --recursive
```

React reconciler 번들을 생성합니다. 이 결과물은 CMake 빌드 후 실행 파일의 `assets/platform` 경로로 복사됩니다.

```powershell
cd Source/Javascript
pnpm install
pnpm build
cd ../..
```

이후 Visual Studio Developer PowerShell 또는 MSVC 환경이 잡힌 터미널에서 네이티브 타깃을 빌드합니다.

```powershell
cmake -S . -B build -DBUILD_TEST=ON
cmake --build build --config Debug
```

빌드가 끝나면 예제 실행 파일 `test2`와 정적 라이브러리 `MachiUi`가 생성됩니다. 테스트를 실행하려면 다음 명령을 사용합니다.

```powershell
ctest --test-dir build --output-on-failure
```


## 🔧 Technical Details


엔진의 구성요소의 기본단위는 서비스(IService)를 상속한다.
제공되는 기본 랜더러가 아닌 커스텀한 DX12/Opengl/Vulkan/Metal 랜더러를 쓰기 위해서는 사용자는 독자적인 IWindowHost를 구현해야한다.

### Modular Logging
MachI-UI는 엔진 통합을 위해 **추상화된 로깅 레이어**를 제공합니다.
- `spdlog` 기반의 자체 로깅 지원 (Standalone)
- 외부 엔진 로거 주입 가능 (Embedded)
- QuickJS 내부 런타임 에러와 React 로그의 통합 관리

# MIT License

Copyright (c) 2026 fabyday

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
