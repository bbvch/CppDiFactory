#include <iostream>
#include <memory>
#include <string>

#include "CppDiFactory.h"

using std::cout;
using std::endl;
using std::shared_ptr;
using std::string;

using CppDiFactory::DiFactory;

class IScrew
{
public:
    virtual bool tight() const = 0;
    virtual ~IScrew() = default;
};

class IWheels
{
public:
    virtual bool inflated() const = 0;
    virtual ~IWheels() = default;
};

class IEngine
{
public:
    virtual double getVolume() const = 0;
    virtual ~IEngine() = default;
    virtual shared_ptr<IScrew> screw() const = 0;
};

class ICar
{
public:
    virtual void startIgnition() = 0;
    virtual ~ICar() = default;

    virtual shared_ptr<IScrew> screw() const = 0;
    virtual shared_ptr<IEngine> engine() const = 0;
};


class Engine : public IEngine
{
public:
    Engine(shared_ptr<IScrew> screw)
    : _screw(screw)
    {}

    virtual double getVolume() const override
    {
        return 10.5;
    }

    virtual shared_ptr<IScrew> screw() const
    {
        return _screw;
    }

private:
    shared_ptr<IScrew>  _screw;
};

class Wheels : public IWheels
{
public:
    virtual bool inflated() const override
    {
        return false;
    }
};

class Screw : public IScrew
{
public:
    virtual bool tight() const override
    {
        return true;
    }
};

class Car : public ICar
{
public:
    Car(shared_ptr<IEngine> engine, shared_ptr<IWheels> wheels, shared_ptr<IScrew> screw)
    : _engine(engine), _wheels(wheels), _screw(screw)
    {}

    virtual void startIgnition()
    {
        std::cout << "starting ignition, engine volume: " << _engine->getVolume()
                  << ", wheels inflated? " << _wheels->inflated() << std::endl;
    }

    virtual shared_ptr<IScrew> screw() const
    {
        return _screw;
    }

    virtual shared_ptr<IEngine> engine() const
    {
        return _engine;
    }

private:
    shared_ptr<IEngine> _engine;
    shared_ptr<IWheels> _wheels;
    shared_ptr<IScrew>  _screw;
};


int main(int argc, const char * argv[]) {
    DiFactory injector;

    // Here's an example of a more complex graph of objects tied together
    // using Injector -

    injector.registerInstancePerRequest<Screw>().withInterfaces<IScrew>();

    // Engine
    injector.registerSingleton<Engine, IScrew>().withInterfaces<IEngine>();

    // Wheels
    injector.registerClass<Wheels>().withInterfaces<IWheels>();

    // Register car with its dependencies - IEngine and IWheels (required by its ctor)
    injector.registerClass<Car, IEngine, IWheels, IScrew>().withInterfaces<ICar>();

    // Car is instansiated with instances of Engine, Wheels and Car automatically!
    auto car = injector.getInstance<ICar>();
    car->startIgnition();

    auto car2 = injector.getInstance<ICar>();


    if(car->engine() != car2->engine())
        cout << "wrong: car.engine != car2.engine\n";

    if (car->screw() == car2->screw())
        cout << "wrong: car.screw == car2.screw\n";

    if (car2->screw() != car2->engine()->screw())
        cout << "wrong: car.screw != car.engine.screw\n";

    return 0;
}

