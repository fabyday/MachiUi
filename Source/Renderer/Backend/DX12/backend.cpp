#include "Core/IWindowHost.h"
#include "Core/UiEngine.h"
#include "Core/ServiceRegistry.h"
#include "Core/ViewManger.h"
#include "Core/SceneGraph.h"
#include "Core/SceneManager.h"
#include "Core/Element.h"
#include "Common/macro.h"
#include "Renderer/IRenderer.h"
#include "Renderer/RenderQueue.h"

// std lib
#include <unordered_map>
//

// OS dependant part
//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <d3dcompiler.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <optional>
#include <string>
#include <wrl.h>
#include <vector>
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib") // 셰이더 컴파일이 필요하다면 추가
#pragma comment(lib, "dxguid.lib")      // 특정 GUID 사용 시 필요

// remove conflict
#ifdef DrawText
#undef DrawText
#endif

class ServiceProvider;

struct Dx12Context
{
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12CommandQueue> commandQueue;
  ComPtr<IDXGIFactory7> dxgiFactory;
  //
  ComPtr<ID3D12Fence> fence;
  HANDLE fenceEvent;
  uint64_t fenceValue = 0;
};

struct NativeWindowContext
{
  ComPtr<IDXGISwapChain4> swapChain;
  std::vector<ComPtr<ID3D12Resource>> renderTargets;
};

MACHI_UI_STATIC bool initializeDx12Context(Dx12Context *out);
MACHI_UI_STATIC void finalizeDx12Context();

/// @brief
class Dx12RendererImpl : public IRenderer
{

  std::unordered_map<uint32_t, IWindow::NativeHandle> nativeHandleMap;

  IWindowHost *winHost;
  UiEngine *engine;
  ViewManager *viewManager;
  SceneManager *sceneManager;
  Dx12Context dx12Context;
  std::unordered_map<uint64_t, ViewId> sceneViewMap;

public:
  virtual void onInit(ServiceProvider *engine) override;

  virtual void enqueueRenderCommand(const RenderCommand &cmd) override;
  virtual void attachScene(uint64_t sceneGraphId, ViewId viewId) override;

  /// @brief
  /// @param queue
  virtual void execute() override;
};

static std::string toLower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
                 { return static_cast<char>(std::tolower(c)); });
  return value;
}

static std::optional<std::string> getColorAttribute(Element *element)
{
  if (element == nullptr)
  {
    return std::nullopt;
  }

  if (const auto *value = element->getAttribute("color"))
  {
    if (const auto *color = std::get_if<std::string>(value))
    {
      return *color;
    }
  }

  return std::nullopt;
}

static std::optional<std::string> findColorAttribute(Element *element)
{
  auto ownColor = getColorAttribute(element);
  if (ownColor.has_value())
  {
    return ownColor;
  }

  if (element == nullptr)
  {
    return std::nullopt;
  }

  for (Element *child : element->getChildren())
  {
    auto color = findColorAttribute(child);
    if (color.has_value())
    {
      return color;
    }
  }

  return std::nullopt;
}

static COLORREF parseColor(const std::string &color)
{
  const std::string normalized = toLower(color);
  if (normalized == "black")
  {
    return RGB(0, 0, 0);
  }
  if (normalized == "white")
  {
    return RGB(255, 255, 255);
  }
  if (normalized == "red")
  {
    return RGB(255, 0, 0);
  }
  if (normalized == "green")
  {
    return RGB(0, 128, 0);
  }
  if (normalized == "blue")
  {
    return RGB(0, 0, 255);
  }
  if (normalized == "yellow" || normalized == "yello")
  {
    return RGB(255, 230, 0);
  }
  if (normalized == "whilte")
  {
    return RGB(255, 255, 255);
  }
  if (normalized.size() == 7 && normalized[0] == '#')
  {
    try
    {
      const int r = std::stoi(normalized.substr(1, 2), nullptr, 16);
      const int g = std::stoi(normalized.substr(3, 2), nullptr, 16);
      const int b = std::stoi(normalized.substr(5, 2), nullptr, 16);
      return RGB(r, g, b);
    }
    catch (...)
    {
    }
  }

  return RGB(240, 240, 240);
}

static void fillRect(HDC dc, const RECT &rect, COLORREF color)
{
  HBRUSH brush = CreateSolidBrush(color);
  FillRect(dc, &rect, brush);
  DeleteObject(brush);
}

static RECT insetRect(RECT rect, int inset)
{
  rect.left += inset;
  rect.top += inset;
  rect.right -= inset;
  rect.bottom -= inset;
  if (rect.right < rect.left)
  {
    rect.right = rect.left;
  }
  if (rect.bottom < rect.top)
  {
    rect.bottom = rect.top;
  }
  return rect;
}

static void drawElement(HDC dc, Element *element, RECT rect, int depth)
{
  if (element == nullptr)
  {
    return;
  }

  auto color = getColorAttribute(element);
  if (color.has_value())
  {
    fillRect(dc, rect, parseColor(color.value()));
  }

  const auto &children = element->getChildren();
  if (children.empty())
  {
    return;
  }

  RECT contentRect = insetRect(rect, depth == 0 ? 0 : 48);
  const int gap = 18;
  const int availableHeight = contentRect.bottom - contentRect.top;
  const int childHeight = children.size() == 1
                              ? availableHeight
                              : (availableHeight - gap * static_cast<int>(children.size() - 1)) / static_cast<int>(children.size());

  for (size_t i = 0; i < children.size(); ++i)
  {
    RECT childRect = contentRect;
    childRect.top = contentRect.top + static_cast<int>(i) * (childHeight + gap);
    childRect.bottom = childRect.top + childHeight;
    drawElement(dc, children[i], childRect, depth + 1);
  }
}

static void drawSceneToWindow(IWindow *window, SceneGraph *graph)
{
  if (window == nullptr || window->getNativeHandle() == nullptr)
  {
    return;
  }

  HWND hwnd = static_cast<HWND>(window->getNativeHandle());
  HDC dc = GetDC(hwnd);
  if (dc == nullptr)
  {
    return;
  }

  RECT rect;
  GetClientRect(hwnd, &rect);
  fillRect(dc, rect, RGB(240, 240, 240));

  if (graph != nullptr)
  {
    drawElement(dc, graph->getRoot(), rect, 0);
  }

  ReleaseDC(hwnd, dc);
}

void Dx12RendererImpl::onInit(ServiceProvider *provider)
{
  // this->engine = engine;
  // Get Window Host
  this->viewManager = provider->getService<ViewManager>();
  this->sceneManager = provider->getService<SceneManager>();

  if (!initializeDx12Context(&dx12Context))
  {
    // error
  }
}

void Dx12RendererImpl::enqueueRenderCommand(const RenderCommand &cmd)
{
  renderQueue.recordCommand(cmd.target, std::get<Color>(cmd.data));
}

void Dx12RendererImpl::attachScene(uint64_t sceneGraphId, ViewId viewId)
{
  sceneViewMap[sceneGraphId] = viewId;
}

void Dx12RendererImpl::execute()
{
  if (viewManager != nullptr && sceneManager != nullptr)
  {
    for (const auto &[sceneGraphId, viewId] : sceneViewMap)
    {
      SceneGraph *graph = sceneManager->getSceneGraph(sceneGraphId);
      IWindow *window = viewManager->getWindowByViewId(viewId);
      drawSceneToWindow(window, graph);
    }
  }

  auto commands = renderQueue.GetCommands();
  for (auto cmd : commands)
  {
    if (this->viewManager->validate(cmd.target))
    {
    }
  }
};

MACHI_UI_STATIC bool initializeDx12Context(Dx12Context *out)
{
  UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
  // 1. 디버그 레이어 활성화 (에러 메시지를 콘솔에 뿌려줌 - 개발 필수!)
  ComPtr<ID3D12Debug> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
  {
    debugController->EnableDebugLayer();
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }
#endif

  // 2. DXGI Factory 생성
  if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&out->dxgiFactory))))
  {
    return false;
  }

  // 3. 고성능 하드웨어 어댑터(GPU) 선택 및 Device 생성
  // D3D_FEATURE_LEVEL_12_0 이상을 지원하는 첫 번째 어댑터를 찾습니다.
  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&out->device))))
  {
    // 실무에서는 여기서 어댑터를 순회하며 적절한 것을 찾는 로직이 추가로 필요할 수 있습니다.
    // 현재는 시스템 기본 어댑터를 사용하도록 설정했습니다.
  }

  // 4. Command Queue 생성
  D3D12_COMMAND_QUEUE_DESC queueDesc = {};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

  if (FAILED(out->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&out->commandQueue))))
  {
    return false;
  };

  // 5. Fence 및 FenceEvent 생성 (CPU-GPU 동기화 도구)
  if (FAILED(out->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&out->fence))))
  {
    return false;
  }

  out->fenceValue = 1;

  // 동기화 시 기다릴 때 사용할 이벤트 핸들
  out->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (out->fenceEvent == nullptr)
  {
    if (FAILED(HRESULT_FROM_WIN32(GetLastError())))
    {
      return false;
    }
  }

  return true;
}

REGISTER_UI_COMPONENT_AS(Dx12RendererImpl, IRenderer, ServicePhase::Render)
