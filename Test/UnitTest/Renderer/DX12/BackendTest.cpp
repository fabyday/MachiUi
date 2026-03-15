#include <gtest/gtest.h>

#include <Windows.h>
#include <d3dcompiler.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <vector>
using Microsoft::WRL::ComPtr;

struct Dx12Context
{
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12CommandQueue> commandQueue;
    ComPtr<IDXGIFactory7> dxgiFactory;
    //
    ComPtr<ID3D12Fence> fence;
    HANDLE fenceEvent;
    uint64_t globalFenceValue = 0;
};

     bool initializeDx12Context(Dx12Context *out);

TEST(Dx12InitTest, CreateDeviceSuccess)
{
    Dx12Context context;

    // 1. 초기화 함수 호출
    initializeDx12Context(&context);

    // 2. 검증: Device와 Factory가 할당되었는가?
    EXPECT_NE(context.device.Get(), nullptr);
    EXPECT_NE(context.dxgiFactory.Get(), nullptr);

    // 3. 기능 확인: 기능 레벨 지원 여부 등
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    HRESULT hr = context.device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
    EXPECT_EQ(hr, S_OK);
}