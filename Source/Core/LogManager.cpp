#include "ComponentRegistry.h"
#include "LogManager.h"
#include "DefaultLogger.h"

void LogManager::onInit(UiEngine *engine)
{

#ifdef STANDALONE_MODE
    defaultLogger = new DefaultLogger();
#else
    defaultLogger = new DefaultLogger();
    // defaultLogger = new NullLogger();
#endif
}


void LogManager::setCustomLogger(ILogger *logger)
{

    // this is SImple Test Code
    if (this->defaultLogger != nullptr)
    {
        delete this->defaultLogger;
    }
    this->defaultLogger = logger;
}

ILogger *LogManager::getLogger()
{
    return this->defaultLogger;
}

REGISTER_UI_COMPONENT(LogManager, ServicePhase::System);