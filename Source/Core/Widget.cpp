#include "Widget.h"

void Widget::AddChild(std::shared_ptr<Widget> child)
{
    children.push_back(child);
    YGNodeInsertChild(node, child->node, (uint32_t)children.size() - 1);
}
void Widget::OnLayout() {}
void Widget::OnPaint(RenderQueue &queue, float parentX, float parentY)
{
    float x = YGNodeLayoutGetLeft(node) + parentX;
    float y = YGNodeLayoutGetTop(node) + parentY;
    float w = YGNodeLayoutGetWidth(node);
    float h = YGNodeLayoutGetHeight(node);

    queue.PushDrawRect({x, y, w, h}, backgroundColor);

    for (auto &child : children)
    {
        child->OnPaint(queue, x, y);
    }
}
