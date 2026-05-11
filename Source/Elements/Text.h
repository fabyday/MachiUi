#pragma once

#include "../Core/Element.h"
#include <algorithm>

class TextElement : public Element
{
public:
    TextElement(uint64_t uid) : Element(uid)
    {
        YGNodeSetMeasureFunc(getLayoutNode(), [](YGNodeConstRef node, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
                             {
                                 auto *element = static_cast<Element *>(YGNodeGetContext(node));
                                 if (element == nullptr)
                                 {
                                     return YGSize{0.0f, 0.0f};
                                 }

                                 float fontSize = 16.0f;
                                 if (const auto *fontSizeValue = element->getAttribute("fontSize"))
                                 {
                                     if (const auto *intValue = std::get_if<int>(fontSizeValue))
                                     {
                                         fontSize = static_cast<float>(*intValue);
                                     }
                                     else if (const auto *floatValue = std::get_if<float>(fontSizeValue))
                                     {
                                         fontSize = *floatValue;
                                     }
                                     else if (const auto *stringValue = std::get_if<std::string>(fontSizeValue))
                                     {
                                         try
                                         {
                                             fontSize = std::stof(*stringValue);
                                         }
                                         catch (...)
                                         {
                                         }
                                     }
                                 }

                                 float lineHeight = fontSize * 1.35f;
                                 if (const auto *lineHeightValue = element->getAttribute("lineHeight"))
                                 {
                                     if (const auto *intValue = std::get_if<int>(lineHeightValue))
                                     {
                                         lineHeight = static_cast<float>(*intValue);
                                     }
                                     else if (const auto *floatValue = std::get_if<float>(lineHeightValue))
                                     {
                                         lineHeight = *floatValue;
                                     }
                                     else if (const auto *stringValue = std::get_if<std::string>(lineHeightValue))
                                     {
                                         try
                                         {
                                             lineHeight = std::stof(*stringValue);
                                         }
                                         catch (...)
                                         {
                                         }
                                     }
                                 }

                                 const float measuredWidth = std::max(1.0f, static_cast<float>(element->getText().size()) * fontSize * 0.56f);
                                 const float measuredHeight = std::max(1.0f, lineHeight);

                                 float resultWidth = measuredWidth;
                                 float resultHeight = measuredHeight;
                                 if (widthMode == YGMeasureModeExactly)
                                 {
                                     resultWidth = width;
                                 }
                                 else if (widthMode == YGMeasureModeAtMost)
                                 {
                                     resultWidth = std::min(measuredWidth, width);
                                 }

                                 if (heightMode == YGMeasureModeExactly)
                                 {
                                     resultHeight = height;
                                 }
                                 else if (heightMode == YGMeasureModeAtMost)
                                 {
                                     resultHeight = std::min(measuredHeight, height);
                                 }

                                 return YGSize{resultWidth, resultHeight};
                             });
    }
};
