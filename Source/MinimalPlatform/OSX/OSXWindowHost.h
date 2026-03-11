#pragma once
#include "../../Core/IWindowHost.h"
#include <vector>



class OSXWindowHost : public IWindowHost{
    std::vector<IWindow*> windowLists;
public:
    OSXWindowHost();
    virtual ~OSXWindowHost() override;

    // IService interface implementation
    void onInit(UiEngine *engine) override;

    // IWindowHost interface implementation
    IWindow * requestWindow() override;

};