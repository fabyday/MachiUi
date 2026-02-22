#include "quickjs.h"
#include <iostream>
#include <string>

// JavaScript에서 호출될 실제 C++ 로직
static JSValue js_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    for (int i = 0; i < argc; i++)
    {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str)
        {
            std::cout << str << (i == argc - 1 ? "" : " ");
            JS_FreeCString(ctx, str);
        }
    }
    std::cout << std::endl;
    return JS_UNDEFINED;
}
class MachiContext
{
    JSRuntime *rt;
    JSContext *ctx;

public:
    MachiContext()
    {
        rt = JS_NewRuntime();
        ctx = JS_NewContext(rt);
        setupConsole(); // 콘솔 설정 호출
    }

    ~MachiContext()
    {
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
    }
    void setupConsole()
    {
        JSValue global_obj = JS_GetGlobalObject(ctx);

        // 1. 'console' 이라는 이름의 새 객체 생성
        JSValue console = JS_NewObject(ctx);

        // 2. 'log'라는 속성에 위에서 만든 C++ 함수(js_print) 연결
        // JS_NewCFunction(컨텍스트, 함수포인터, 함수이름, 인자개수)
        JS_SetPropertyStr(ctx, console, "log",
                          JS_NewCFunction(ctx, js_print, "log", 1));

        // 3. 전역 객체에 'console' 등록
        JS_SetPropertyStr(ctx, global_obj, "console", console);

        JS_FreeValue(ctx, global_obj);
    }

    void eval(const std::string &code)
    {
        JSValue result = JS_Eval(ctx, code.c_str(), code.length(), "<input>", JS_EVAL_TYPE_GLOBAL);

        if (JS_IsException(result))
        {
            JSValue exception = JS_GetException(ctx);
            const char *str = JS_ToCString(ctx, exception);
            std::cerr << "JS Error: " << str << std::endl;
            JS_FreeCString(ctx, str);
            JS_FreeValue(ctx, exception);
        }

        JS_FreeValue(ctx, result);
    }
};
int main()
{
    MachiContext machi;
    machi.eval("const a = 10; const b = 20; console.log('Sum is: ' + (a + b));");
    machi.eval("document.getElementById('app').innerHTML = '<h1>Hello, MachiUi!</h1>';");
    return 0;
}