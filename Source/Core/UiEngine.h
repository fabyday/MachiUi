#pragma once
#include <vector>
#include <memory>
#include "IService.h"
#include "IWindowHost.h"
#include "ITimer.h"
class UiEngine
{
private:
    bool engineInitFlag = false;

protected:
    // construct All Components Objects,
    void _bootstrapComponent();
    // call onInit for all components, this is where dependency injection happens
    void _initializeComponents();


    void setupFundamentalServices();

    //
    void _updateLayout();


public:
    UiEngine();
    ~UiEngine();

    // 엔진 가동: 부품 조립 및 초기화
    void Init();

    // 메인 루프: 모든 부품의 Update 호출
    void Run();
    void finalize();



    // Internal engine update logic. 
    // Do not invoke manually in standalone mode.
    void update(double deltaTime);
    




    template <typename T>
    T *GetService()
    {
        for (auto &component : m_components)
        {
            if (auto casted = dynamic_cast<T *>(component.get()))
            {
                return casted;
            }
        }
        return nullptr;
    }

private:

std::vector<std::unique_ptr<IService>> m_components;
    IWindowHost * windowHost;
    ITimer* timer;
};