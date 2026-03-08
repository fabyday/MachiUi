#pragma once
#include "IService.h"
#include "ILogger.h"

class LogManager : public IService
{

    ILogger *defaultLogger;

public:
    virtual ~LogManager() = default;

    // 엔진이 초기화될 때 호출 (여기서 다른 컴포넌트를 찾거나 초기 설정을 합니다)
    virtual void onInit(UiEngine *engine) override;

    // global logger setter, getter
    void setCustomLogger(ILogger *logger);
    ILogger *getLogger();
};