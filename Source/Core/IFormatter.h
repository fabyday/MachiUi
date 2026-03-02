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
    // const std::type_info &type;
    std::function<void(const void *, IArgVisitor &)> applier;
};

template <typename T>
inline RawArg make_arg(const T &val)
{
    return {&val, [](const void *p, IArgVisitor &v)
            {
                const T &data = *static_cast<const T *>(p);

                // compile time check Type
                if constexpr (std::is_integral_v<T>)
                {
                    v.Visit(static_cast<int>(data));
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    v.Visit(static_cast<double>(data));
                }
                else if constexpr (std::is_convertible_v<T, std::string_view>)
                {
                    v.Visit(std::string_view(data));
                }
                else
                {
                    // 처리할 수 없는 타입일 경우 컴파일 에러 발생
                    static_assert(sizeof(T) == 0, "Unsupported type for IArgVisitor");
                }
            }};
};

class IFormatter
{
public:
    virtual ~IFormatter() = default;

    virtual std::string Format(const char *fmt, const std::vector<RawArg> &args) = 0;
};