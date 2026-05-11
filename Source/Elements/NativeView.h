#pragma once

#include "../Core/Element.h"

class NativeViewElement : public Element
{
public:
    explicit NativeViewElement(uint64_t uid) : Element(uid)
    {
    }
};
