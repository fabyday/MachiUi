
struct JSCFunctionListEntry;
#include <cstddef>

class NativeBinder
{
public:
    virtual ~NativeBinder() = default;

    // JSContext* 대신 void*를 쓰거나, 전방 선언만 활용
    virtual void Bind(void *ctx) = 0;
};
struct BinderMeta
{
    const char *className;
    const JSCFunctionListEntry *functions;
    size_t numFunctions;
    const char *parentClassName; // 상속을 지원하기 위한 필드
};





class GenericBinder : public NativeBinder
{
    BinderMeta meta;

public:
    GenericBinder(const BinderMeta &m) : meta(m) {}
    virtual void Bind(void *ctx) override;
};
