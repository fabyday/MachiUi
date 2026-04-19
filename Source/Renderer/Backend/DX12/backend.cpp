#include "Core/IWindowHost.h"
#include "Core/UiEngine.h"
#include "Core/ServiceRegistry.h"
#include "Core/ViewManger.h"
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
#include <d3dcompiler.h>
#include <d3d12.h>
#include <dxgi1_6.h>
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

  std::unordered_map<uint32_t, void *> nativeHandleMap;

  IWindowHost *winHost;
  UiEngine *engine;
  ViewManager *viewManager;
  Dx12Context dx12Context;

public:
  virtual void onInit(ServiceProvider *engine) override;

  virtual void enqueueRenderCommand(const RenderCommand &cmd) override;

  /// @brief
  /// @param queue
  virtual void execute() override;
};

void Dx12RendererImpl::onInit(ServiceProvider *provider)
{
  // this->engine = engine;
  // Get Window Host
  this->viewManager = provider->getService<ViewManager>();

  if (!initializeDx12Context(&dx12Context))
  {
    // error
  }
}

void Dx12RendererImpl::enqueueRenderCommand(const RenderCommand &cmd)
{
  renderQueue.recordCommand(cmd.target, std::get<Color>(cmd.data));
}

void Dx12RendererImpl::execute()
{
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