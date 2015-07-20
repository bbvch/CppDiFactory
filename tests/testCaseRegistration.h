#ifndef TESTCASEREGISTRATION_H
#define TESTCASEREGISTRATION_H

#include "CppDiFactory.h"

namespace testCaseRegistration
{

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

TEST_CASE( "Registration: Create unknown type", "Assert should be thrown when creating an unknown class" ){

    CppDiFactory::DiFactory myFactory;
    CHECK(myFactory.isValid() == true);
}

TEST_CASE( "De-Registration: Register class", "Assert should be thrown when creating an unknown class" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerSingleton<Engine>();

    CHECK(myFactory.isValid() == true);
}

TEST_CASE( "De-Registration: Register class with interface", "Assert should be thrown when creating an unknown class" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerSingleton<Engine>().withInterfaces<IEngine>();

    CHECK(myFactory.isValid() == true);
}

TEST_CASE( "De-Registration: Unregister class", "Assert should be thrown when creating an unknown class" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerSingleton<Engine>().withInterfaces<IEngine>();

    myFactory.unregister<Engine>();

    CHECK(myFactory.isValid() == false);
}

TEST_CASE( "Registration: Unregister interface", "Assert should be thrown when creating an unknown class" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerSingleton<Engine>().withInterfaces<IEngine>();

    myFactory.unregister<IEngine>();

    CHECK(myFactory.isValid() == true);
}

}

#endif // TESTCASEREGISTRATION_H

