#pragma once
#include <vector>
#include <memory>
#include "Core/IComponent.h"

class UiEngine
{


    protected:
    // construct All Components Objects, 
    void _bootstrapComponent();
    // call onInit for all components, this is where dependency injection happens
    void _initializeComponents();
public:
    UiEngine();
    ~UiEngine();

    // 엔진 가동: 부품 조립 및 초기화
    void Init();

    

    // 메인 루프: 모든 부품의 Update 호출
    void Run();

    template <typename T>
    T *GetComponent()
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
    // 실제로 생성된 부품들을 관리하는 바구니
    std::vector<std::unique_ptr<IComponent>> m_components;
};