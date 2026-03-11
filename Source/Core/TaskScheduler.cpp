#include "TaskScheduler.h"
#include "ComponentRegistry.h"
#include "Core/UiEngine.h"


void TaskScheduler::onInit(UiEngine* engine){

}
void TaskScheduler::postDelayTask(){
    
}


REGISTER_UI_COMPONENT(TaskScheduler, ServicePhase::Logic)