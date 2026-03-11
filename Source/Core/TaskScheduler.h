#pragma once 

#include "Core/IService.h"
#include <functional>
#include <algorithm>
#include <vector>

class TaskScheduler : public IService{ 
  
    
public:

    // Managers should call this method.
    void postDelayTask();

    //run on engine 
    void processReservedTask();
    
    void onInit(UiEngine *engine) override;

    void update();

private : 



};

