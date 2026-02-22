#include <string>
#include <vector>
#include <memory>
#include <yoga/Yoga.h>
#include "../Renderer/RenderCommand.h"
#include "../Renderer/RenderQueue.h"

class Widget
{

public:
    YGNodeRef node;

    std::string id;
    std::vector<std::shared_ptr<Widget>> children;
    Color backgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};
    
    Widget()
    {
        node = YGNodeNew();
    }

    ~Widget()
    {
        YGNodeFree(node);
    }

    void AddChild(std::shared_ptr<Widget> child);

    virtual void OnLayout();

    virtual void OnPaint(RenderQueue &queue, float parentX, float parentY);
};