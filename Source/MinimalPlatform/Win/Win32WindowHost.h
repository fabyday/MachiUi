#pragma once
#include "../../Core/IWindowHost.h"




class Win32WindowHost : public IWindowHost{
    void * opaque;
    
public:
    Win32WindowHost();
    virtual ~Win32WindowHost();

    // IService interface implementation
    void onInit(UiEngine *engine) override;

    // IWindowHost interface implementation
    IWindow *requestWindow() override;

};