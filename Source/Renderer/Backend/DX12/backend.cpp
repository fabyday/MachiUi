#include "Core/IWindowHost.h"
#include "Core/UiEngine.h"
#include "Core/ServiceRegistry.h"
#include "Core/ViewManger.h"
#include "Core/SceneGraph.h"
#include "Core/SceneManager.h"
#include "Core/Element.h"
#include "Elements/Text.h"
#include "Elements/NativeView.h"
#include "Common/macro.h"
#include "Renderer/IRenderer.h"
#include "Renderer/RenderQueue.h"
#include "Core/NativeViewRegistry.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstring>
#include <d3d12.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <d3dcompiler.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <dxgi1_6.h>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dxguid.lib")

class ServiceProvider;

struct Dx12Context
{
  ComPtr<ID3D12Device> device;
  ComPtr<ID3D12CommandQueue> commandQueue;
  ComPtr<IDXGIFactory7> dxgiFactory;
  ComPtr<ID3D12Fence> fence;
  HANDLE fenceEvent = nullptr;
  uint64_t fenceValue = 1;
};

namespace
{
constexpr UINT FrameCount = 2;
constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

struct RgbaColor
{
  float r;
  float g;
  float b;
  float a;
};

struct SolidVertex
{
  float position[2];
  float color[4];
};

struct TextDrawCommand
{
  std::wstring text;
  D2D1_RECT_F rect;
  RgbaColor color;
  float fontSize;
  std::wstring fontFamily;
  DWRITE_FONT_WEIGHT fontWeight;
  DWRITE_TEXT_ALIGNMENT textAlignment;
  float lineHeight;
};

struct Dx12Pipeline
{
  ComPtr<ID3D12RootSignature> rootSignature;
  ComPtr<ID3D12PipelineState> pipelineState;
};

struct DxTextContext
{
  ComPtr<ID3D11Device> d3d11Device;
  ComPtr<ID3D11DeviceContext> d3d11Context;
  ComPtr<ID3D11On12Device> d3d11On12Device;
  ComPtr<ID2D1Factory1> d2dFactory;
  ComPtr<ID2D1Device> d2dDevice;
  ComPtr<ID2D1DeviceContext> d2dContext;
  ComPtr<IDWriteFactory> dwriteFactory;
  bool initialized = false;
};

struct NativeWindowContext
{
  ComPtr<IDXGISwapChain4> swapChain;
  ComPtr<ID3D12DescriptorHeap> rtvHeap;
  ComPtr<ID3D12CommandAllocator> commandAllocator;
  ComPtr<ID3D12GraphicsCommandList> commandList;
  ComPtr<ID3D12Resource> vertexBuffer;
  std::vector<ComPtr<ID3D11Resource>> wrappedBackBuffers;
  std::vector<ComPtr<ID2D1Bitmap1>> d2dTargets;
  std::vector<ComPtr<ID3D12Resource>> renderTargets;
  UINT64 vertexBufferCapacity = 0;
  UINT rtvDescriptorSize = 0;
  UINT width = 0;
  UINT height = 0;
  bool initialized = false;
};
} // namespace

MACHI_UI_STATIC bool initializeDx12Context(Dx12Context *out);

class Dx12RendererImpl : public IRenderer
{
  ViewManager *viewManager = nullptr;
  SceneManager *sceneManager = nullptr;
  Dx12Context dx12Context;
  Dx12Pipeline pipeline;
  DxTextContext textContext;
  std::unordered_map<uint64_t, ViewId> sceneViewMap;
  NativeViewRegistry *nativeViewRegistry = nullptr;
  std::unordered_map<HWND, NativeWindowContext> windowContexts;
  std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

public:
  ~Dx12RendererImpl() override;

  void onInit(ServiceProvider *provider) override;
  void enqueueRenderCommand(const RenderCommand &cmd) override;
  void attachScene(uint64_t sceneGraphId, ViewId viewId) override;
  void execute() override;
};

namespace
{
static std::string toLower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
                 { return static_cast<char>(std::tolower(c)); });
  return value;
}

static std::optional<std::string> getStringAttribute(Element *element, const char *key)
{
  if (element == nullptr)
  {
    return std::nullopt;
  }

  if (const auto *value = element->getAttribute(key))
  {
    if (const auto *text = std::get_if<std::string>(value))
    {
      return *text;
    }
  }

  return std::nullopt;
}

static std::optional<std::string> getBackgroundColorAttribute(Element *element)
{
  if (auto color = getStringAttribute(element, "backgroundColor"))
  {
    return color;
  }
  if (auto color = getStringAttribute(element, "background"))
  {
    return color;
  }
  return std::nullopt;
}

static std::string getNativeViewStringAttribute(Element *element, const char *primaryKey, const char *secondaryKey, const std::string &fallback)
{
  if (auto primary = getStringAttribute(element, primaryKey))
  {
    if (!primary->empty())
    {
      return *primary;
    }
  }
  if (secondaryKey != nullptr)
  {
    if (auto secondary = getStringAttribute(element, secondaryKey))
    {
      if (!secondary->empty())
      {
        return *secondary;
      }
    }
  }
  return fallback;
}

static std::optional<std::string> getTextColorAttribute(Element *element)
{
  return getStringAttribute(element, "color");
}

static float getFloatAttribute(Element *element, const char *key, float fallback)
{
  if (element == nullptr)
  {
    return fallback;
  }

  const auto *value = element->getAttribute(key);
  if (value == nullptr)
  {
    return fallback;
  }

  if (const auto *intValue = std::get_if<int>(value))
  {
    return static_cast<float>(*intValue);
  }
  if (const auto *floatValue = std::get_if<float>(value))
  {
    return *floatValue;
  }
  if (const auto *stringValue = std::get_if<std::string>(value))
  {
    try
    {
      return std::stof(*stringValue);
    }
    catch (...)
    {
    }
  }

  return fallback;
}

static DWRITE_FONT_WEIGHT getFontWeightAttribute(Element *element, DWRITE_FONT_WEIGHT fallback)
{
  if (element == nullptr)
  {
    return fallback;
  }

  const auto *value = element->getAttribute("fontWeight");
  if (value == nullptr)
  {
    return fallback;
  }

  int weight = 0;
  if (const auto *intValue = std::get_if<int>(value))
  {
    weight = *intValue;
  }
  else if (const auto *floatValue = std::get_if<float>(value))
  {
    weight = static_cast<int>(*floatValue);
  }
  else if (const auto *stringValue = std::get_if<std::string>(value))
  {
    const std::string normalized = toLower(*stringValue);
    if (normalized == "bold")
    {
      return DWRITE_FONT_WEIGHT_BOLD;
    }
    if (normalized == "semibold" || normalized == "semi-bold")
    {
      return DWRITE_FONT_WEIGHT_SEMI_BOLD;
    }
    if (normalized == "medium")
    {
      return DWRITE_FONT_WEIGHT_MEDIUM;
    }
    if (normalized == "light")
    {
      return DWRITE_FONT_WEIGHT_LIGHT;
    }

    try
    {
      weight = std::stoi(normalized);
    }
    catch (...)
    {
      return fallback;
    }
  }

  if (weight <= 0)
  {
    return fallback;
  }
  if (weight < 350)
  {
    return DWRITE_FONT_WEIGHT_LIGHT;
  }
  if (weight < 500)
  {
    return DWRITE_FONT_WEIGHT_NORMAL;
  }
  if (weight < 650)
  {
    return DWRITE_FONT_WEIGHT_SEMI_BOLD;
  }
  return DWRITE_FONT_WEIGHT_BOLD;
}

static DWRITE_TEXT_ALIGNMENT getTextAlignmentAttribute(Element *element, DWRITE_TEXT_ALIGNMENT fallback)
{
  if (auto align = getStringAttribute(element, "textAlign"))
  {
    const std::string normalized = toLower(align.value());
    if (normalized == "center")
    {
      return DWRITE_TEXT_ALIGNMENT_CENTER;
    }
    if (normalized == "right" || normalized == "end")
    {
      return DWRITE_TEXT_ALIGNMENT_TRAILING;
    }
    if (normalized == "justify")
    {
      return DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
    }
    if (normalized == "left" || normalized == "start")
    {
      return DWRITE_TEXT_ALIGNMENT_LEADING;
    }
  }

  return fallback;
}

static RgbaColor multiplyAlpha(RgbaColor color, float alpha)
{
  color.a *= std::clamp(alpha, 0.0f, 1.0f);
  return color;
}

static float getElementOpacity(Element *element, float elapsedSeconds)
{
  float opacity = getFloatAttribute(element, "opacity", 1.0f);
  if (auto animation = getStringAttribute(element, "animation"))
  {
    const std::string normalized = toLower(animation.value());
    if (normalized.find("pulse") != std::string::npos)
    {
      constexpr float Pi = 3.14159265358979323846f;
      const float wave = 0.5f + 0.5f * std::sin((elapsedSeconds / 1.6f) * Pi * 2.0f);
      opacity *= 0.55f + wave * 0.45f;
    }
  }

  return std::clamp(opacity, 0.0f, 1.0f);
}

static std::pair<float, float> parseTransformTranslate(const std::string &transform)
{
  const auto open = transform.find('(');
  const auto close = transform.find(')', open == std::string::npos ? 0 : open);
  if (open == std::string::npos || close == std::string::npos || close <= open + 1)
  {
    return {0.0f, 0.0f};
  }

  std::string values = transform.substr(open + 1, close - open - 1);
  std::replace(values.begin(), values.end(), ',', ' ');

  std::istringstream stream(values);
  float x = 0.0f;
  float y = 0.0f;
  stream >> x;
  stream >> y;
  return {x, y};
}

static std::pair<float, float> getTransformOffset(Element *element)
{
  float x = getFloatAttribute(element, "translateX", 0.0f);
  float y = getFloatAttribute(element, "translateY", 0.0f);

  if (auto transform = getStringAttribute(element, "transform"))
  {
    const auto offset = parseTransformTranslate(transform.value());
    x += offset.first;
    y += offset.second;
  }

  return {x, y};
}

static std::wstring utf8ToWide(const std::string &text)
{
  if (text.empty())
  {
    return {};
  }

  const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
  if (size <= 0)
  {
    return std::wstring(text.begin(), text.end());
  }

  std::wstring result(static_cast<size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size);
  return result;
}

static RgbaColor rgb(unsigned char r, unsigned char g, unsigned char b)
{
  return RgbaColor{
      static_cast<float>(r) / 255.0f,
      static_cast<float>(g) / 255.0f,
      static_cast<float>(b) / 255.0f,
      1.0f};
}

static RgbaColor parseColor(const std::string &color)
{
  const std::string normalized = toLower(color);
  if (normalized == "black")
  {
    return rgb(0, 0, 0);
  }
  if (normalized == "white" || normalized == "whilte")
  {
    return rgb(255, 255, 255);
  }
  if (normalized == "red")
  {
    return rgb(255, 0, 0);
  }
  if (normalized == "green")
  {
    return rgb(0, 128, 0);
  }
  if (normalized == "blue")
  {
    return rgb(0, 0, 255);
  }
  if (normalized == "yellow" || normalized == "yello")
  {
    return rgb(255, 230, 0);
  }
  if ((normalized.size() == 7 || normalized.size() == 9) && normalized[0] == '#')
  {
    try
    {
      const int r = std::stoi(normalized.substr(1, 2), nullptr, 16);
      const int g = std::stoi(normalized.substr(3, 2), nullptr, 16);
      const int b = std::stoi(normalized.substr(5, 2), nullptr, 16);
      RgbaColor result = rgb(static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b));
      if (normalized.size() == 9)
      {
        const int a = std::stoi(normalized.substr(7, 2), nullptr, 16);
        result.a = static_cast<float>(a) / 255.0f;
      }
      return result;
    }
    catch (...)
    {
    }
  }

  return rgb(240, 240, 240);
}

static float toNdcX(LONG x, UINT width)
{
  return (static_cast<float>(x) / static_cast<float>(width)) * 2.0f - 1.0f;
}

static float toNdcY(LONG y, UINT height)
{
  return 1.0f - (static_cast<float>(y) / static_cast<float>(height)) * 2.0f;
}

static void appendRectVertices(std::vector<SolidVertex> &vertices, const RECT &rect, UINT width, UINT height, RgbaColor color)
{
  if (rect.right <= rect.left || rect.bottom <= rect.top || width == 0 || height == 0)
  {
    return;
  }

  const float left = toNdcX(rect.left, width);
  const float right = toNdcX(rect.right, width);
  const float top = toNdcY(rect.top, height);
  const float bottom = toNdcY(rect.bottom, height);
  const float c[4] = {color.r, color.g, color.b, color.a};

  vertices.push_back({{left, top}, {c[0], c[1], c[2], c[3]}});
  vertices.push_back({{right, top}, {c[0], c[1], c[2], c[3]}});
  vertices.push_back({{left, bottom}, {c[0], c[1], c[2], c[3]}});
  vertices.push_back({{left, bottom}, {c[0], c[1], c[2], c[3]}});
  vertices.push_back({{right, top}, {c[0], c[1], c[2], c[3]}});
  vertices.push_back({{right, bottom}, {c[0], c[1], c[2], c[3]}});
}

static void collectElementDrawData(
    Element *element,
    float parentX,
    float parentY,
    UINT width,
    UINT height,
    std::vector<SolidVertex> &vertices,
    std::vector<TextDrawCommand> &textCommands,
    std::vector<NativeViewSlot> &nativeViewSlots,
    RgbaColor inheritedTextColor,
    float inheritedFontSize,
    const std::wstring &inheritedFontFamily,
    DWRITE_FONT_WEIGHT inheritedFontWeight,
    DWRITE_TEXT_ALIGNMENT inheritedTextAlignment,
    float inheritedLineHeight,
    float inheritedOpacity,
    float elapsedSeconds)
{
  if (element == nullptr || !element->getVisible())
  {
    return;
  }

  YGNodeRef layoutNode = element->getLayoutNode();
  const auto transformOffset = getTransformOffset(element);
  const float x = parentX + YGNodeLayoutGetLeft(layoutNode) + transformOffset.first;
  const float y = parentY + YGNodeLayoutGetTop(layoutNode) + transformOffset.second;
  const float elementWidth = YGNodeLayoutGetWidth(layoutNode);
  const float elementHeight = YGNodeLayoutGetHeight(layoutNode);

  RECT rect{
      static_cast<LONG>(std::lround(x)),
      static_cast<LONG>(std::lround(y)),
      static_cast<LONG>(std::lround(x + elementWidth)),
      static_cast<LONG>(std::lround(y + elementHeight))};

  const float opacity = inheritedOpacity * getElementOpacity(element, elapsedSeconds);

  auto backgroundColor = getBackgroundColorAttribute(element);
  if (backgroundColor.has_value())
  {
    appendRectVertices(vertices, rect, width, height, multiplyAlpha(parseColor(backgroundColor.value()), opacity));
  }

  if (dynamic_cast<NativeViewElement *>(element) != nullptr)
  {
    NativeViewSlot slot;
    slot.elementId = element->getUid();
    slot.nativeType = getNativeViewStringAttribute(element, "nativeViewType", "viewType", "native-view");
    slot.viewId = getNativeViewStringAttribute(element, "viewId", "id", std::to_string(element->getUid()));
    slot.rect = Rect{x, y, elementWidth, elementHeight};
    slot.visible = element->getVisible() && opacity > 0.0f;
    slot.zIndex = static_cast<int>(getFloatAttribute(element, "zIndex", 0.0f));
    nativeViewSlots.push_back(slot);
  }

  RgbaColor textColor = inheritedTextColor;
  if (auto color = getTextColorAttribute(element))
  {
    textColor = parseColor(color.value());
  }
  const RgbaColor drawTextColor = multiplyAlpha(textColor, opacity);

  float fontSize = getFloatAttribute(element, "fontSize", inheritedFontSize);
  DWRITE_FONT_WEIGHT fontWeight = getFontWeightAttribute(element, inheritedFontWeight);
  DWRITE_TEXT_ALIGNMENT textAlignment = getTextAlignmentAttribute(element, inheritedTextAlignment);
  float lineHeight = getFloatAttribute(element, "lineHeight", inheritedLineHeight > 0.0f ? inheritedLineHeight : fontSize * 1.35f);
  std::wstring fontFamily = inheritedFontFamily;
  if (auto family = getStringAttribute(element, "fontFamily"))
  {
    fontFamily = utf8ToWide(family.value());
  }

  const std::string &text = element->getText();
  if (!text.empty())
  {
    const float textHeight = std::max(elementHeight, lineHeight);
    textCommands.push_back(TextDrawCommand{
        utf8ToWide(text),
        D2D1::RectF(x, y, x + std::max(elementWidth, 1.0f), y + textHeight),
        drawTextColor,
        fontSize,
        fontFamily,
        fontWeight,
        textAlignment,
        lineHeight});
  }

  const auto &children = element->getChildren();
  if (children.empty())
  {
    return;
  }

  for (Element *child : children)
  {
    collectElementDrawData(
        child,
        x,
        y,
        width,
        height,
        vertices,
        textCommands,
        nativeViewSlots,
        textColor,
        fontSize,
        fontFamily,
        fontWeight,
        textAlignment,
        lineHeight,
        opacity,
        elapsedSeconds);
  }
}

static bool getClientSize(HWND hwnd, UINT *width, UINT *height)
{
  RECT rect{};
  if (!GetClientRect(hwnd, &rect))
  {
    return false;
  }

  const LONG clientWidth = rect.right - rect.left;
  const LONG clientHeight = rect.bottom - rect.top;
  if (clientWidth <= 0 || clientHeight <= 0)
  {
    return false;
  }

  *width = static_cast<UINT>(clientWidth);
  *height = static_cast<UINT>(clientHeight);
  return true;
}

static bool createSolidPipeline(Dx12Context *context, Dx12Pipeline *pipeline)
{
  if (context == nullptr || pipeline == nullptr || context->device == nullptr)
  {
    return false;
  }

  const char *shaderSource = R"(
struct VSInput
{
  float2 position : POSITION;
  float4 color : COLOR;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
  PSInput output;
  output.position = float4(input.position, 0.0f, 1.0f);
  output.color = input.color;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  return input.color;
}
)";

  UINT compileFlags = 0;
#if defined(_DEBUG)
  compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  ComPtr<ID3DBlob> vertexShader;
  ComPtr<ID3DBlob> pixelShader;
  ComPtr<ID3DBlob> errors;

  if (FAILED(D3DCompile(shaderSource, std::strlen(shaderSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &errors)))
  {
    return false;
  }
  if (FAILED(D3DCompile(shaderSource, std::strlen(shaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &errors)))
  {
    return false;
  }

  D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
  rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ComPtr<ID3DBlob> signature;
  if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors)))
  {
    return false;
  }
  if (FAILED(context->device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pipeline->rootSignature))))
  {
    return false;
  }

  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(SolidVertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SolidVertex, color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };

  D3D12_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
  rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rasterizerDesc.DepthClipEnable = TRUE;

  D3D12_BLEND_DESC blendDesc{};
  blendDesc.RenderTarget[0].BlendEnable = TRUE;
  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
  blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;

  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  depthStencilDesc.DepthEnable = FALSE;
  depthStencilDesc.StencilEnable = FALSE;

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
  pipelineDesc.InputLayout = {inputLayout, _countof(inputLayout)};
  pipelineDesc.pRootSignature = pipeline->rootSignature.Get();
  pipelineDesc.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
  pipelineDesc.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
  pipelineDesc.RasterizerState = rasterizerDesc;
  pipelineDesc.BlendState = blendDesc;
  pipelineDesc.DepthStencilState = depthStencilDesc;
  pipelineDesc.SampleMask = UINT_MAX;
  pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pipelineDesc.NumRenderTargets = 1;
  pipelineDesc.RTVFormats[0] = BackBufferFormat;
  pipelineDesc.SampleDesc.Count = 1;

  return SUCCEEDED(context->device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipeline->pipelineState)));
}

static void waitForGpu(Dx12Context *context)
{
  if (context == nullptr || context->commandQueue == nullptr || context->fence == nullptr || context->fenceEvent == nullptr)
  {
    return;
  }

  const uint64_t fence = context->fenceValue++;
  if (FAILED(context->commandQueue->Signal(context->fence.Get(), fence)))
  {
    return;
  }

  if (context->fence->GetCompletedValue() < fence)
  {
    if (SUCCEEDED(context->fence->SetEventOnCompletion(fence, context->fenceEvent)))
    {
      WaitForSingleObject(context->fenceEvent, INFINITE);
    }
  }
}

static bool createTextContext(Dx12Context *context, DxTextContext *textContext)
{
  if (context == nullptr || textContext == nullptr || context->device == nullptr || context->commandQueue == nullptr)
  {
    return false;
  }

  UINT d3d11Flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

  IUnknown *queues[] = {context->commandQueue.Get()};
  if (FAILED(D3D11On12CreateDevice(
          context->device.Get(),
          d3d11Flags,
          nullptr,
          0,
          queues,
          1,
          0,
          &textContext->d3d11Device,
          &textContext->d3d11Context,
          nullptr)))
  {
    return false;
  }

  if (FAILED(textContext->d3d11Device.As(&textContext->d3d11On12Device)))
  {
    return false;
  }

  D2D1_FACTORY_OPTIONS factoryOptions{};
#if defined(_DEBUG)
  factoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

  if (FAILED(D2D1CreateFactory(
          D2D1_FACTORY_TYPE_SINGLE_THREADED,
          __uuidof(ID2D1Factory1),
          &factoryOptions,
          reinterpret_cast<void **>(textContext->d2dFactory.GetAddressOf()))))
  {
    return false;
  }

  ComPtr<IDXGIDevice> dxgiDevice;
  if (FAILED(textContext->d3d11On12Device.As(&dxgiDevice)))
  {
    return false;
  }

  if (FAILED(textContext->d2dFactory->CreateDevice(dxgiDevice.Get(), &textContext->d2dDevice)))
  {
    return false;
  }

  if (FAILED(textContext->d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &textContext->d2dContext)))
  {
    return false;
  }

  if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown **>(textContext->dwriteFactory.GetAddressOf()))))
  {
    return false;
  }

  textContext->initialized = true;
  return true;
}

static void resetTextTargets(NativeWindowContext *windowContext)
{
  windowContext->d2dTargets.clear();
  windowContext->wrappedBackBuffers.clear();
}

static bool createTextTargets(DxTextContext *textContext, NativeWindowContext *windowContext)
{
  if (textContext == nullptr || !textContext->initialized || windowContext == nullptr)
  {
    return false;
  }

  resetTextTargets(windowContext);
  windowContext->wrappedBackBuffers.resize(FrameCount);
  windowContext->d2dTargets.resize(FrameCount);

  D3D11_RESOURCE_FLAGS d3d11Flags{};
  d3d11Flags.BindFlags = D3D11_BIND_RENDER_TARGET;

  const D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
      D2D1::PixelFormat(BackBufferFormat, D2D1_ALPHA_MODE_IGNORE));

  for (UINT i = 0; i < FrameCount; ++i)
  {
    if (FAILED(textContext->d3d11On12Device->CreateWrappedResource(
            windowContext->renderTargets[i].Get(),
            &d3d11Flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            IID_PPV_ARGS(&windowContext->wrappedBackBuffers[i]))))
    {
      resetTextTargets(windowContext);
      return false;
    }

    ComPtr<IDXGISurface> surface;
    if (FAILED(windowContext->wrappedBackBuffers[i].As(&surface)))
    {
      resetTextTargets(windowContext);
      return false;
    }

    if (FAILED(textContext->d2dContext->CreateBitmapFromDxgiSurface(
            surface.Get(),
            bitmapProperties,
            &windowContext->d2dTargets[i])))
    {
      resetTextTargets(windowContext);
      return false;
    }
  }

  return true;
}

static bool createRenderTargets(Dx12Context *context, NativeWindowContext *windowContext)
{
  D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
  rtvHeapDesc.NumDescriptors = FrameCount;
  rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  if (FAILED(context->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&windowContext->rtvHeap))))
  {
    return false;
  }

  windowContext->rtvDescriptorSize = context->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  windowContext->renderTargets.resize(FrameCount);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = windowContext->rtvHeap->GetCPUDescriptorHandleForHeapStart();
  for (UINT i = 0; i < FrameCount; ++i)
  {
    if (FAILED(windowContext->swapChain->GetBuffer(i, IID_PPV_ARGS(&windowContext->renderTargets[i]))))
    {
      return false;
    }

    context->device->CreateRenderTargetView(windowContext->renderTargets[i].Get(), nullptr, rtvHandle);
    rtvHandle.ptr += windowContext->rtvDescriptorSize;
  }

  return true;
}

static bool createWindowContext(Dx12Context *context, Dx12Pipeline *pipeline, DxTextContext *textContext, HWND hwnd, UINT width, UINT height, NativeWindowContext *windowContext)
{
  if (pipeline == nullptr || pipeline->pipelineState == nullptr)
  {
    return false;
  }

  DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
  swapChainDesc.BufferCount = FrameCount;
  swapChainDesc.Width = width;
  swapChainDesc.Height = height;
  swapChainDesc.Format = BackBufferFormat;
  swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapChainDesc.SampleDesc.Count = 1;
  swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

  ComPtr<IDXGISwapChain1> swapChain;
  if (FAILED(context->dxgiFactory->CreateSwapChainForHwnd(
          context->commandQueue.Get(),
          hwnd,
          &swapChainDesc,
          nullptr,
          nullptr,
          &swapChain)))
  {
    return false;
  }

  context->dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

  if (FAILED(swapChain.As(&windowContext->swapChain)))
  {
    return false;
  }

  if (FAILED(context->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&windowContext->commandAllocator))))
  {
    return false;
  }

  if (FAILED(context->device->CreateCommandList(
          0,
          D3D12_COMMAND_LIST_TYPE_DIRECT,
          windowContext->commandAllocator.Get(),
          pipeline->pipelineState.Get(),
          IID_PPV_ARGS(&windowContext->commandList))))
  {
    return false;
  }
  windowContext->commandList->Close();

  windowContext->width = width;
  windowContext->height = height;
  windowContext->initialized = createRenderTargets(context, windowContext);
  if (windowContext->initialized && textContext != nullptr && textContext->initialized)
  {
    createTextTargets(textContext, windowContext);
  }
  return windowContext->initialized;
}

static bool resizeWindowContext(Dx12Context *context, DxTextContext *textContext, NativeWindowContext *windowContext, UINT width, UINT height)
{
  if (windowContext->width == width && windowContext->height == height)
  {
    return true;
  }

  waitForGpu(context);

  for (auto &renderTarget : windowContext->renderTargets)
  {
    renderTarget.Reset();
  }
  resetTextTargets(windowContext);

  if (FAILED(windowContext->swapChain->ResizeBuffers(FrameCount, width, height, BackBufferFormat, 0)))
  {
    return false;
  }

  windowContext->width = width;
  windowContext->height = height;
  if (!createRenderTargets(context, windowContext))
  {
    return false;
  }
  if (textContext != nullptr && textContext->initialized)
  {
    createTextTargets(textContext, windowContext);
  }
  return true;
}

static bool ensureVertexBuffer(Dx12Context *context, NativeWindowContext *windowContext, UINT64 requiredSize)
{
  if (requiredSize == 0)
  {
    return true;
  }

  if (windowContext->vertexBuffer != nullptr && windowContext->vertexBufferCapacity >= requiredSize)
  {
    return true;
  }

  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
  heapProps.CreationNodeMask = 1;
  heapProps.VisibleNodeMask = 1;

  const UINT64 bufferSize = std::max<UINT64>(requiredSize, 4096);

  D3D12_RESOURCE_DESC bufferDesc{};
  bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  bufferDesc.Width = bufferSize;
  bufferDesc.Height = 1;
  bufferDesc.DepthOrArraySize = 1;
  bufferDesc.MipLevels = 1;
  bufferDesc.SampleDesc.Count = 1;
  bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  if (FAILED(context->device->CreateCommittedResource(
          &heapProps,
          D3D12_HEAP_FLAG_NONE,
          &bufferDesc,
          D3D12_RESOURCE_STATE_GENERIC_READ,
          nullptr,
          IID_PPV_ARGS(windowContext->vertexBuffer.ReleaseAndGetAddressOf()))))
  {
    return false;
  }

  windowContext->vertexBufferCapacity = bufferSize;
  return true;
}

static bool uploadVertices(NativeWindowContext *windowContext, const std::vector<SolidVertex> &vertices, D3D12_VERTEX_BUFFER_VIEW *vertexBufferView)
{
  const UINT64 bufferSize = static_cast<UINT64>(sizeof(SolidVertex) * vertices.size());
  void *mappedData = nullptr;
  D3D12_RANGE readRange{0, 0};
  if (FAILED(windowContext->vertexBuffer->Map(0, &readRange, &mappedData)))
  {
    return false;
  }

  std::memcpy(mappedData, vertices.data(), static_cast<size_t>(bufferSize));
  windowContext->vertexBuffer->Unmap(0, nullptr);

  vertexBufferView->BufferLocation = windowContext->vertexBuffer->GetGPUVirtualAddress();
  vertexBufferView->SizeInBytes = static_cast<UINT>(bufferSize);
  vertexBufferView->StrideInBytes = sizeof(SolidVertex);
  return true;
}

static bool drawTextOverlay(DxTextContext *textContext, NativeWindowContext *windowContext, UINT frameIndex, const std::vector<TextDrawCommand> &textCommands)
{
  if (textContext == nullptr || !textContext->initialized || windowContext == nullptr ||
      frameIndex >= windowContext->wrappedBackBuffers.size() || frameIndex >= windowContext->d2dTargets.size())
  {
    return false;
  }

  ID3D11Resource *wrappedResources[] = {windowContext->wrappedBackBuffers[frameIndex].Get()};
  textContext->d3d11On12Device->AcquireWrappedResources(wrappedResources, 1);

  textContext->d2dContext->SetTarget(windowContext->d2dTargets[frameIndex].Get());
  textContext->d2dContext->BeginDraw();

  for (const auto &command : textCommands)
  {
    if (command.text.empty())
    {
      continue;
    }

    ComPtr<IDWriteTextFormat> textFormat;
    if (FAILED(textContext->dwriteFactory->CreateTextFormat(
            command.fontFamily.empty() ? L"Segoe UI" : command.fontFamily.c_str(),
            nullptr,
            command.fontWeight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            command.fontSize,
            L"ko-kr",
            &textFormat)))
    {
      continue;
    }

    textFormat->SetTextAlignment(command.textAlignment);
    textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    if (command.lineHeight > 0.0f)
    {
      textFormat->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, command.lineHeight, std::min(command.fontSize, command.lineHeight));
    }

    ComPtr<ID2D1SolidColorBrush> brush;
    if (FAILED(textContext->d2dContext->CreateSolidColorBrush(
            D2D1::ColorF(command.color.r, command.color.g, command.color.b, command.color.a),
            &brush)))
    {
      continue;
    }

    textContext->d2dContext->DrawText(
        command.text.c_str(),
        static_cast<UINT32>(command.text.size()),
        textFormat.Get(),
        command.rect,
        brush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_CLIP,
        DWRITE_MEASURING_MODE_NATURAL);
  }

  const HRESULT drawResult = textContext->d2dContext->EndDraw();
  textContext->d3d11On12Device->ReleaseWrappedResources(wrappedResources, 1);
  textContext->d3d11Context->Flush();
  return SUCCEEDED(drawResult);
}

static bool renderVertices(Dx12Context *context, Dx12Pipeline *pipeline, DxTextContext *textContext, NativeWindowContext *windowContext, const std::vector<SolidVertex> &vertices, const std::vector<TextDrawCommand> &textCommands)
{
  if (!windowContext->initialized || pipeline == nullptr || pipeline->pipelineState == nullptr || pipeline->rootSignature == nullptr)
  {
    return false;
  }

  const UINT frameIndex = windowContext->swapChain->GetCurrentBackBufferIndex();
  const bool canDrawText = textContext != nullptr && textContext->initialized &&
                           frameIndex < windowContext->wrappedBackBuffers.size() &&
                           frameIndex < windowContext->d2dTargets.size();

  if (FAILED(windowContext->commandAllocator->Reset()))
  {
    return false;
  }
  if (FAILED(windowContext->commandList->Reset(windowContext->commandAllocator.Get(), pipeline->pipelineState.Get())))
  {
    return false;
  }

  D3D12_RESOURCE_BARRIER toRenderTarget{};
  toRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  toRenderTarget.Transition.pResource = windowContext->renderTargets[frameIndex].Get();
  toRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
  toRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  toRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  windowContext->commandList->ResourceBarrier(1, &toRenderTarget);

  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = windowContext->rtvHeap->GetCPUDescriptorHandleForHeapStart();
  rtvHandle.ptr += static_cast<SIZE_T>(frameIndex) * windowContext->rtvDescriptorSize;

  const float clearColor[4] = {17.0f / 255.0f, 24.0f / 255.0f, 39.0f / 255.0f, 1.0f};
  windowContext->commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
  windowContext->commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

  if (!vertices.empty())
  {
    const UINT64 vertexBufferSize = static_cast<UINT64>(sizeof(SolidVertex) * vertices.size());
    if (!ensureVertexBuffer(context, windowContext, vertexBufferSize))
    {
      return false;
    }

    D3D12_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(windowContext->width);
    viewport.Height = static_cast<float>(windowContext->height);
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorRect{0, 0, static_cast<LONG>(windowContext->width), static_cast<LONG>(windowContext->height)};
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    if (!uploadVertices(windowContext, vertices, &vertexBufferView))
    {
      return false;
    }

    windowContext->commandList->SetGraphicsRootSignature(pipeline->rootSignature.Get());
    windowContext->commandList->RSSetViewports(1, &viewport);
    windowContext->commandList->RSSetScissorRects(1, &scissorRect);
    windowContext->commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    windowContext->commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    windowContext->commandList->DrawInstanced(static_cast<UINT>(vertices.size()), 1, 0, 0);
  }

  if (!canDrawText)
  {
    D3D12_RESOURCE_BARRIER toPresent{};
    toPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toPresent.Transition.pResource = windowContext->renderTargets[frameIndex].Get();
    toPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    toPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    windowContext->commandList->ResourceBarrier(1, &toPresent);
  }

  if (FAILED(windowContext->commandList->Close()))
  {
    return false;
  }

  ID3D12CommandList *commandLists[] = {windowContext->commandList.Get()};
  context->commandQueue->ExecuteCommandLists(1, commandLists);

  if (canDrawText)
  {
    drawTextOverlay(textContext, windowContext, frameIndex, textCommands);
  }

  const HRESULT presentResult = windowContext->swapChain->Present(1, 0);
  waitForGpu(context);
  return SUCCEEDED(presentResult);
}

static bool renderSceneToWindow(Dx12Context *context, Dx12Pipeline *pipeline, DxTextContext *textContext, NativeViewRegistry *nativeViewRegistry, NativeWindowContext *windowContext, HWND hwnd, SceneGraph *graph, float elapsedSeconds)
{
  UINT width = 0;
  UINT height = 0;
  if (!getClientSize(hwnd, &width, &height))
  {
    return false;
  }

  if (!windowContext->initialized)
  {
    if (!createWindowContext(context, pipeline, textContext, hwnd, width, height, windowContext))
    {
      return false;
    }
  }
  else if (!resizeWindowContext(context, textContext, windowContext, width, height))
  {
    return false;
  }

  std::vector<SolidVertex> vertices;
  std::vector<TextDrawCommand> textCommands;
  std::vector<NativeViewSlot> nativeViewSlots;
  if (graph != nullptr && graph->getRoot() != nullptr)
  {
    Element *root = graph->getRoot();
    YGNodeRef rootNode = root->getLayoutNode();
    YGNodeStyleSetWidth(rootNode, static_cast<float>(width));
    YGNodeStyleSetHeight(rootNode, static_cast<float>(height));
    YGNodeCalculateLayout(rootNode, static_cast<float>(width), static_cast<float>(height), YGDirectionLTR);
    collectElementDrawData(
        root,
        0.0f,
        0.0f,
        width,
        height,
        vertices,
        textCommands,
        nativeViewSlots,
        rgb(31, 42, 68),
        16.0f,
        L"Segoe UI",
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_TEXT_ALIGNMENT_LEADING,
        0.0f,
        1.0f,
        elapsedSeconds);
  }

  if (nativeViewRegistry != nullptr)
  {
    for (const auto &slot : nativeViewSlots)
    {
      nativeViewRegistry->syncSlot(slot);
    }
  }

  return renderVertices(context, pipeline, textContext, windowContext, vertices, textCommands);
}
} // namespace

Dx12RendererImpl::~Dx12RendererImpl()
{
  waitForGpu(&dx12Context);
  if (dx12Context.fenceEvent != nullptr)
  {
    CloseHandle(dx12Context.fenceEvent);
    dx12Context.fenceEvent = nullptr;
  }
}

void Dx12RendererImpl::onInit(ServiceProvider *provider)
{
  this->viewManager = provider->getService<ViewManager>();
  this->sceneManager = provider->getService<SceneManager>();
  this->nativeViewRegistry = provider->getService<NativeViewRegistry>();

  if (!initializeDx12Context(&dx12Context))
  {
    return;
  }

  createSolidPipeline(&dx12Context, &pipeline);
  createTextContext(&dx12Context, &textContext);
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
  if (viewManager == nullptr || sceneManager == nullptr || dx12Context.device == nullptr || pipeline.pipelineState == nullptr)
  {
    return;
  }

  const auto now = std::chrono::steady_clock::now();
  const float elapsedSeconds = std::chrono::duration<float>(now - startTime).count();

  if (nativeViewRegistry != nullptr)
  {
    nativeViewRegistry->beginSync();
  }

  for (const auto &[sceneGraphId, viewId] : sceneViewMap)
  {
    SceneGraph *graph = sceneManager->getSceneGraph(sceneGraphId);
    IWindow *window = viewManager->getWindowByViewId(viewId);
    if (window == nullptr || window->getNativeHandle() == nullptr)
    {
      continue;
    }

    HWND hwnd = static_cast<HWND>(window->getNativeHandle());
    renderSceneToWindow(&dx12Context, &pipeline, &textContext, nativeViewRegistry, &windowContexts[hwnd], hwnd, graph, elapsedSeconds);
  }

  if (nativeViewRegistry != nullptr)
  {
    nativeViewRegistry->endSync();
  }
}

MACHI_UI_STATIC bool initializeDx12Context(Dx12Context *out)
{
  if (out == nullptr)
  {
    return false;
  }

  UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
  ComPtr<ID3D12Debug> debugController;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
  {
    debugController->EnableDebugLayer();
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
  }
#endif

  if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&out->dxgiFactory))))
  {
    return false;
  }

  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&out->device))))
  {
    return false;
  }

  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

  if (FAILED(out->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&out->commandQueue))))
  {
    return false;
  }

  if (FAILED(out->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&out->fence))))
  {
    return false;
  }

  out->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (out->fenceEvent == nullptr)
  {
    return false;
  }

  return true;
}

REGISTER_UI_COMPONENT_AS(Dx12RendererImpl, IRenderer, ServicePhase::Render)
