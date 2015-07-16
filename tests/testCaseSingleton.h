#ifndef TESTCASESINGLETON_H
#define TESTCASESINGLETON_H

#include "CppDiFactory.h"
class IEngine
{
public:
	virtual double getVolume() const = 0;
	virtual ~IEngine() = default;
};

class Engine : public IEngine
{
public:
	virtual double getVolume() const override
	{
		return 10.5;
	}
};



TEST_CASE( "Singleton Test", "Check if two instances of a registered singleton are equal" ){

	CppDiFactory::DiFactory myFactory;
	myFactory.registerSingleton<Engine>().withInterfaces<IEngine>();

	auto engine  = myFactory.getInstance<IEngine>();
	auto engine2 = myFactory.getInstance<IEngine>();

	CHECK(engine == engine2);
}

#endif // TESTCASESINGLETON_H

