#pragma once
#include "../../Core/IWindowHost.h"




class OSXWindowHost : public IWindowHost{
    void * opaque;
    
public:
    OSXWindowHost();
    virtual ~OSXWindowHost();

    // IService interface implementation
    void onInit(UiEngine *engine) override;

    // IWindowHost interface implementation
    IWindow * requestWindow() override;

};