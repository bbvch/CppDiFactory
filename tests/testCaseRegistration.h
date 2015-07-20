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

class Engine : public IEngine
{
public:
    virtual double getVolume() const override
    {
        return 10.5;
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

class Screw : public IScrew
{
public:
    Screw(std::shared_ptr<IEngine> engine):
        _engine(engine)
    {}
    virtual bool tight() const override
    {
        return true;
    }
private:
    std::shared_ptr<IEngine> _engine;
};

TEST_CASE( "Registration: Create unknown type", "" ){

    CppDiFactory::DiFactory myFactory;
    CHECK_NOTHROW(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<Engine>());
}

TEST_CASE( "De-Registration: Register class", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<Engine>());
}

TEST_CASE( "De-Registration: Register class with interface", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine>().withInterfaces<IEngine>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<IEngine>());
}

TEST_CASE( "De-Registration: Unregister class", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine>().withInterfaces<IEngine>();

    myFactory.unregister<Engine>();

    CHECK_THROWS(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IEngine>());
}

TEST_CASE( "Registration: Unregister interface", "" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerClass<Engine>().withInterfaces<IEngine>();

    myFactory.unregister<IEngine>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IEngine>());
}

TEST_CASE( "Registration: Register instance", "" ){

    CppDiFactory::DiFactory myFactory;

     myFactory.registerInstance<Engine>(std::make_shared<Engine>()).withInterfaces<IEngine>();

    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<IEngine>());
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

    myFactory.registerSingleton<Screw, IEngine>().withInterfaces<IScrew>();
    myFactory.registerInstancePerRequest<Engine>().withInterfaces<IEngine>();


    CHECK_THROWS(myFactory.validate());
    CHECK_THROWS(myFactory.getInstance<IScrew>());
}

TEST_CASE( "Registration: singleton dependent on SIPR", "Should not be valid" ){

    CppDiFactory::DiFactory myFactory;

    myFactory.registerInstancePerRequest<Screw, IEngine>().withInterfaces<IScrew>();
    myFactory.registerSingleton<Engine>().withInterfaces<IEngine>();


    CHECK_NOTHROW(myFactory.validate());
    CHECK_NOTHROW(myFactory.getInstance<IEngine>());
}


}

#endif // TESTCASEREGISTRATION_H

