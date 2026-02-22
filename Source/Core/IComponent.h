#pragma once

// 전방 선언: UiEngine의 상세 구현을 몰라도 포인터는 다룰 수 있게 합니다.
class UiEngine;

class IComponent
{
public:
    virtual ~IComponent() = default;

    // 엔진이 초기화될 때 호출 (여기서 다른 컴포넌트를 찾거나 초기 설정을 합니다)
    virtual void OnInit(UiEngine *engine) = 0;

    // 매 프레임 호출
    virtual void OnUpdate() = 0;
};