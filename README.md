# CppDiFactory

A simple C++11 single header dependency injection container

All instances are managed using std::shared_ptr.

The DiFactory allows to register different classes and the interfaces they implement.
There are four different ways the factory can handle classes:

| Name                        | registration method        | Description |
| --------------------------- | -------------------------- | ----------- |
| Regular Class               | registerClass              | Create a new instance each time an instance is required. |
| Singleton Class             | registerInstance           | Always use a predefined instance. |
| Singleton Class on demand   | registerSingleton          | Create an instance the first time it is used and return that instance for each request. Once the instance is not needed anymore (by any user), destroy it again.|
| Single Instance Per Request | registerInstancePerRequest | Similar to regular Class, but when this class is used as a dependency, the same instance will be used for all dependencies of the current request.|

When an instance of an interface is requested, an instance of the class which implements that
interface is returned (this depends on the type of registration of the class).

The DiFactory will validate that all dependencies can be resolved before an object is created. In 
case of an error (missing type, cyclic dependency, ...), an exception is thrown.
To avoid a bigger performance impact, this validation is only done once for each type.

Idea based upon:

http://www.codeproject.com/Articles/567981/AnplusIOCplusContainerplususingplusVariadicplusTem

https://github.com/Autodesk/goatnative-inject.git

##usage

###class definitions
```c++
	ClassA: public IntfA1, IntfA2
	{
	public:
	   ClassA();
	}

	// used as singleton
	ClassB: public IntfB
	{
	public:
	   ClassB(const shared_ptr<IntfA1>& intfA1);

	   const shared_ptr<IntfA1> _a1;
	}

	// used as singleton
	ClassC: public IntfF
	{
	public:
	   ClassC(int x);
	}

	// single instance per request (e.g. all dependencies to IntfC
	// should actually point to the same instance)
	ClassD: public IntfD
	{
	public:
	   ClassC(const shared_ptr<IntfA2>& intfA2);

	   const shared_ptr<IntfA2> _a2;
	}

	ClassE: public IntfE
	{
	public:
	   ClassD(const shared_ptr<IntfD>& intfD, const shared_ptr<IntfB>& intfB, const shared_ptr<IntfC>& intfC);

	   const shared_ptr<IntfD> _d;
	   const shared_ptr<IntfB> _b;
	   const shared_ptr<IntfC> _c;
	}

	ClassF: public IntfF
	{
	public:
	   ClassD(const shared_ptr<IntfE>& intfE, const shared_ptr<IntfD>& intfB);

	   const shared_ptr<IntfE> _e;
	   const shared_ptr<IntfD> _d;
	}
```

###registration
```c++
	diFactory.registerClass<ClassA>().withInterfaces<IntfA1, IntfA2>;
	diFactory.registerSingleton<ClassB, IntfA1>().withInterfaces<MyIntfB>;
	diFactory.registerInstance<ClassC>(make_shared<ClassC>(5)).withInterfaces<IntfB>;
	diFactory.registerInstancePerRequest<ClassD, IntfA2>().withInterfaces<IntfD>;
	diFactory.registerClass<ClassE, IntfD, IntfB, IntfC>().withInterfaces<IntfE>;
	diFactory.registerClass<ClassF, IntfE, IntfB, IntfC>().withInterfaces<IntfF>;
```

###creating objects
```c++
    //Create two instances of IntfF
	shared_ptr<IntfE> f1 = diFactory.getInstance<IntfF>();
	shared_ptr<IntfF> f2 = diFactory.getInstance<IntfF>();
	// the following members are equal:
	// f1._e._b == f2._e._b
	// f1._e._c == f2._e._c
	// f1._d == f1._e._d
	// f2._d == f2._e._d
	// but f1._d != f2._d

	weak_ptr<IntfB> b1 = f1._e._b;
	weak_ptr<IntfC> c1 = f1._e._c;

	// remove all counted references to our singletons
	f1.reset();
	f2.reset();
	
	// b1 is expired (not kept alive by the diFactory)
	// c1 is kept alive by the diFactory

	shared_ptr<IntfF> f3 = diFactory.getInstance<IntfF>();
	shared_ptr<IntfF> f4 = diFactory.getInstance<IntfF>();
	// f3._e._c == f4._e._c == c1 (c1 is kept alive by the diFactory)
	// f3._e._b == f4._e._b
	// but f3._e._b != b1 (b1 is expired)
```
