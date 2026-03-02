#pragma once


// ENGINE CORE HEADER
#include "Core/Widget.h"
#include "Core/UiEngine.h"





// LOGGING HEADER
#include "Core/LogManager.h"
#include "Core/ILogger.h"



// RENDER HEADER
#ifdef EMBEDED
// do nothing
#else // STANDALONE MODE
    
    #include "Renderer/RenderQueue.h"
#endif 