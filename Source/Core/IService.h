#pragma once
class UiEngine;

class IService
{

public:
    virtual ~IService() = default;

    // 서비스 초기화 메서드
    virtual void onInit(UiEngine* engine) = 0;

    // 서비스 종료 메서드
    virtual void Shutdown() = 0;
}