#include <cassert>
#include <memory>

#include "CppDiFactory.h"

using CppDiFactory::DiFactory;

class NullObject
{
};

int main(int argc, const char * argv[]) {
	DiFactory injector;

    // Here's an example of how to register a manually created instance of an object
    auto nullInstance = std::make_shared<NullObject>();

    injector.registerInstance(nullInstance);

    auto null1 = injector.getInstance<NullObject>();
    auto null2 = injector.getInstance<NullObject>();

    assert(null1.get() == null2.get());
    assert(null1.get() == nullInstance.get());
    return 0;
}
