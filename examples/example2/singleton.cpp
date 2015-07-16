#include <cassert>
#include <functional>

#include "CppDiFactory.h"

using CppDiFactory::DiFactory;

class IExecutor
{
public:
    virtual void execute(std::function<void()> func) = 0;
    virtual ~IExecutor() = default;
};

class SyncExecutor : public IExecutor
{
public:
    void execute(std::function<void()> func) override
    {
        func();
    }
};

int main(int argc, const char * argv[]) {
	DiFactory injector;

    // Here we're registering IExecutor with SyncExecutor as singleton
    injector.registerSingleton<SyncExecutor>();
    injector.registerInterface<SyncExecutor, IExecutor>();

    // Whenever we'll ask for IExecutor we'll get the same instance of SyncExecutor
    auto executor1 = injector.getInstance<IExecutor>();
    auto executor2 = injector.getInstance<IExecutor>();

    // Compare returned pointer which are expected to be the same
    assert(executor1.get() == executor2.get());
    return 0;
}
