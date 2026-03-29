#include <memory>

/**
 *
 *
 * @example
 *
 * class SomeFactory : public IDIFactory
 * {
 *
 * public:
 *  SomeFactory(some1, some2){}
 *
 *
 * }
 *
 */

template <typename Derived>
class IDIFactory
{

    template <typename T>
    std::shared_ptr<T> create()
    {
        return static_cast<Derived *>(this)->template create<T>();
    }
};




#define REGISTER_UI_COMPONENT(TYPE) 

