#include "IService.h"

void IService::initialize(ServiceProvider *provider)
{
    if (initFlag)
    {

        return;
    }

    initFlag = true;
    onInit(provider);
}