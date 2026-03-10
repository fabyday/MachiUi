#include "IService.h"


void IService::initialize(UiEngine*engine){
    if(initFlag){

        return;    
    }
    
    initFlag = true;
    onInit(engine);
    
}