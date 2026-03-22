

struct AnyType
{
    template <typename T>
    operator T()
    {
        return T();
    }
};

template <size_t... Is>
struct count_chcker
{
    template <typename T>
    auto operator()(T *) -> decltype((T((void)Is, AnyType{})...))
};