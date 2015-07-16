#include <iostream>
#include "CppDiFactory.h"

using CppDiFactory::DiFactory;

class IHello
{
public:
    virtual void hello() const = 0;
    virtual ~IHello() = default;
};


class Hello : public IHello
{
public:
    virtual void hello() const
    {
        std::cout << "hello world!" << std::endl;
    }
};


int main(int argc, const char** argv)
{
	DiFactory injector;

    // Let's register the concrete class
    injector.registerClass<Hello>();//.withInterfaces<IHello>();

    // Now register IHello with Hello so that each time we ask for IHello 
    // We get Hello -
    injector.registerInterface<Hello, IHello>();

    auto helloInstance = injector.getInstance<IHello>();

    helloInstance->hello();
}
