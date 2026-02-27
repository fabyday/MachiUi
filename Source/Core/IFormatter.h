#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

class IArgVisitor
{
public:
    virtual void Visit(int val) = 0;
    virtual void Visit(double val) = 0;
    virtual void Visit(std::string_view val) = 0;
    // 필요한 타입들을 여기에 추가...
};
struct RawArg
{
    const void *value;
    const std::type_info &type;
    std::function<void(const void *, IArgVisitor &)> applier;
};

template <typename T>
inline RawArg make_arg(const T &val)
{
    return {&val, [](const void *p, IArgVisitor &v)
            {
                v.Visit(*static_cast<const T *>(p))
            }};
};

class IFormatter
{
public:
    virtual ~IFormatter() = default;

    virtual std::string Format(const char *fmt, const std::vector<RawArg> &args) = 0;
};