#pragma once
#include "IService.h"
#include "Types.h"
#include <optional>

class IInput : public IService
{

public:
    virtual ~IInput() = default;

    InputEvent *getInput();

};