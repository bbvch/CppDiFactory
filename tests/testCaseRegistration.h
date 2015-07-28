#ifndef TESTCASEREGISTRATION_H
#define TESTCASEREGISTRATION_H

#include "CppDiFactory.h"

namespace testCaseRegistration
{

class IScrew
{
public:
    virtual bool tight() const = 0;
    virtual ~IScrew() = default;
};

class IEngine
{
public:
    virtual double getVolume() const = 0;
    virtual ~IEngine() = default;
};

class IMotor
{
public:
    virtual double getVolume() const = 0;
    virtual ~IMotor() = default;
};

class IVehicle
{
public:
    virtual double getVolume() const = 0;
    virtual ~IVehicle() = default;
};

class Screw : public IScrew
{
public:
    virtual bool tight() const override
    {
        return true;
    }
};

class Vehicle : public IVehicle
{
public:
    Vehicle(std::shared_ptr<IMotor> motor):
        _motor(motor)
    {}

    virtual double getVolume() const override
    {
        return 10.5;
    }

private:
    std::shared_ptr<IMotor> _motor;
};

class Motor : public IMotor
{
public:
    Motor(std::shared_ptr<IVehicle> vehicle):
        _vehicle(vehicle)
    {}

    virtual double getVolume() const override
    {
        return 10.5;
    }

private:
    std::shared_ptr<IVehicle> _vehicle;
};

class Engine : public IEngine
{
public:
    Engine(std::shared_ptr<IScrew> engine):
        _engine(engine)
    {}
    virtual double getVolume() const override
    {
        return 10.5;
    }
private:
    std::shared_ptr<IScrew> _engine;
};

TEST_CASE( "Registration: Create unknown type", "" ){

    CppDiFactory::DiFactory myFactory;
    CHECK_NOTHROW(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<Screw>());
}

TEST_CASE( "De-Registration: Register class", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Screw>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<Screw>());
}

TEST_CASE( "De-Registration: Register class with interface", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Screw>().withInterfaces<IScrew>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<IScrew>());
}

TEST_CASE( "De-Registration: Unregister class", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Screw>().withInterfaces<IScrew>();

    myFactory.unregister<Screw>();

    CHECK_THROWS(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IScrew>());
}

TEST_CASE( "Registration: Unregister interface", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Screw>().withInterfaces<IScrew>();

    myFactory.unregister<IScrew>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IScrew>());
}

TEST_CASE( "Registration: Register instance", "" ){

    CppDiFactory::DiFactory myFactory;

     myFactory.registerInstance<Screw>(std::make_shared<Screw>()).withInterfaces<IScrew>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<IScrew>());
}

TEST_CASE( "Registration: circular dependencies", "Should not be valid" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Motor, IVehicle>().withInterfaces<IMotor>();
    myFactory.registerClass<Vehicle, IMotor>().withInterfaces<IVehicle>();


    CHECK_THROWS(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IVehicle>());
    CHECK_THROWS(myFactory.getInstance<IMotor>());
}

TEST_CASE( "Registration: SIPR dependent on singleton", "Should not be valid" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerSingleton<Engine, IScrew>().withInterfaces<IEngine>();
    myFactory.registerInstancePerRequest<Screw>().withInterfaces<IScrew>();


    CHECK_THROWS(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IEngine>());
}

TEST_CASE( "Registration: singleton dependent on SIPR", "Should not be valid" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerInstancePerRequest<Engine, IScrew>().withInterfaces<IEngine>();
    myFactory.registerSingleton<Screw>().withInterfaces<IScrew>();


    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<IScrew>());
}

TEST_CASE( "Registration: instance provided at request with param", "Should be valid" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine, IScrew>().withInterfaces<IEngine>();
    myFactory.registerInstanceProvidedAtRequest<Screw>().withInterfaces<IScrew>();


    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW((myFactory.getInstance<IEngine, Screw>(std::make_shared<Screw>())));
}

TEST_CASE( "Registration: instance provided at request without param", "Should fail" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine, IScrew>().withInterfaces<IEngine>();
    myFactory.registerInstanceProvidedAtRequest<Screw>().withInterfaces<IScrew>();


    CHECK_NOTHROW(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IEngine>());
}

TEST_CASE( "Registration: single instance per request with param", "Should be valid" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine, IScrew>().withInterfaces<IEngine>();
    myFactory.registerInstancePerRequest<Screw>().withInterfaces<IScrew>();


    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW((myFactory.getInstance<IEngine, Screw>(std::make_shared<Screw>())));
}

TEST_CASE( "Registration: register instance with param", "Should not be valid" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine, IScrew>().withInterfaces<IEngine>();
    myFactory.registerInterface<Screw, IScrew>();

    std::shared_ptr<Screw> engine1 = std::make_shared<Screw>();
    std::shared_ptr<Screw> engine2 = std::make_shared<Screw>();
    std::shared_ptr<Screw> engine3 = std::make_shared<Screw>();

    myFactory.registerInstance<Screw>(engine1);
    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW((myFactory.getInstance<IEngine>()));
    myFactory.registerInstance<Screw>(engine2);
    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW((myFactory.getInstance<IEngine>()));

    CHECK_THROWS((myFactory.getInstance<IEngine, Screw>(engine3)));
}


}

#endif // TESTCASEREGISTRATION_H

