#pragma once
#include "Core/IService.h"
#include "gmock/gmock.h"
#include <Core/IWindow.h>
#include <Core/IWindowHost.h>
#include <Core/UiEngine.h>
#include <gmock/gmock.h>
#include <map>
#include <typeindex>
using testing::Return;
#include <memory>
#include <vector>

// actually Illegal Aceessor


struct UiEngineAcess {
    // 1. virtual은 절대 넣지 마세요 (UiEngine에 없으므로)
    
    // 2. 첫 번째 멤버: bool
    bool engineInitFlag;

    // 3. 패딩 강제 삽입 (가장 중요!)
    // bool(1) 뒤에 vector(8배수)가 오기 때문에 컴파일러가 7바이트를 비웁니다.
    char padding[7]; 

    // 4. 이제서야 위치가 맞음
    std::vector<std::unique_ptr<IService>> m_components;
};

inline void injectMockService(UiEngine *engine,
                              std::unique_ptr<IService> mockService) {
  auto *proxy = reinterpret_cast<UiEngineAcess *>(engine);
  proxy->m_components.push_back(std::move(mockService));
}

// this Helper Class mimic UiEngine Class and help dependancy injection.
class MockUiEngine : public UiEngine {
public:
  MockUiEngine() {
    // do nothing , just return nullptr
  }

 
};

