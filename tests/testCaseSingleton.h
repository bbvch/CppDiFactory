#ifndef TESTCASESINGLETON_H
#define TESTCASESINGLETON_H

#include "CppDiFactory.h"

namespace testCaseSingleton
{

class IEngine
{
public:
	virtual double getVolume() const = 0;
	virtual ~IEngine() = default;
};

class Engine : public IEngine
{
  private:
    static bool _valid;
public:
    Engine() { _valid = true; }
    virtual ~Engine () { _valid = false; }

	virtual double getVolume() const override
	{
		return 10.5;
	}

	static bool isValid() { return _valid; }
};

bool Engine::_valid = false;

TEST_CASE( "Singleton Test", "Check if two instances of a registered singleton are equal" ){

    CppDiFactory::DiFactory myFactory;
    myFactory.registerSingleton<Engine>().withInterfaces<IEngine>();

    auto engine  = myFactory.getInstance<IEngine>();
    auto engine2 = myFactory.getInstance<IEngine>();

    CHECK(engine == engine2);
}

TEST_CASE( "Singleton Lifetime test", "Check that the lifetime of the singleton is correct (alive while used)" ){

    CppDiFactory::DiFactory myFactory;
    myFactory.registerSingleton<Engine>().withInterfaces<IEngine>();

    std::shared_ptr<IEngine> spEngine;
    std::weak_ptr<IEngine> wpEngine;

    // no request yet --> Engine shouldn't exist yet.
    CHECK(!Engine::isValid());

    //NOTE: use sub-block to ensure that no temporary shared-pointers remain alive
    {
        //First call to getInstance should create the engine
        spEngine = myFactory.getInstance<IEngine>();
        CHECK(Engine::isValid());
    }

    //shared pointer is alive --> engine should still exist
    //NOTE: use sub-block to ensure that no temporary shared-pointers remain alive
    {
        CHECK(Engine::isValid());

        //second call should get same instance
        wpEngine = myFactory.getInstance<IEngine>();
        CHECK(wpEngine.lock() == spEngine);
    }

    //remove the only shared pointer to the engine (--> engine should get destroyed)
    spEngine.reset();

    //shared pointer is not alive anymore --> engine should have been destroyed.
    CHECK(wpEngine.expired());
    CHECK(!Engine::isValid());
}

}

#endif // TESTCASESINGLETON_H

