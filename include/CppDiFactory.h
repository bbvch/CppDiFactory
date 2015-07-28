//
//  CppDiFactory.h
//
//  Created by Daniel Bernhard and Juergen Messerer on 30.06.2015
//  Copyright (c) 2015 Daniel Bernhard and Juergen Messerer.
//  License: Apache License 2.0
//  All rights reserved.


#ifndef CPP_DI_FACTORY_H
#define CPP_DI_FACTORY_H

#include <memory>
#include <mutex>
#include <unordered_map>

/// C++ Dependency Injection Factory
/// Dependency injection container aka Inversion of Control (IoC) container
/// using C++11 and variadic templates.
///
/// Idea based upon:
/// http://www.codeproject.com/Articles/567981/AnplusIOCplusContainerplususingplusVariadicplusTem
/// https://github.com/Autodesk/goatnative-inject.git
namespace CppDiFactory
{
    using std::lock_guard;
    using std::make_shared;
    using std::recursive_mutex;
    using std::shared_ptr;
    using std::size_t;
    using std::static_pointer_cast;
    using std::weak_ptr;
    using std::unordered_map;

    //TODO:
    // 1.) Optional: Check cyclic dependencies during registration.


    /// Custom type ID method that uses an ID of type size_t and not a string (e.g. type name) -
    /// map lookups should go faster than if we would have used RTTI's typeid<T>().name() which returns a string
    /// as key.
    /// Basically we define a template type with a static method and use the address of that method as ID value.
    /// ( Taken from http://codereview.stackexchange.com/questions/44936/unique-type-id-in-c )
    template<typename T>
    struct type { static void id() { } };

    /// Return a unique ID for the type T (use the address of static method as ID)
    template<typename T>
    size_t type_id() { return reinterpret_cast<size_t>(&type<T>::id); }


    /// The DiFactory is an object factory implementing the dependency injection pattern.
    /// All instances are managed using std::shared_ptr.
    /// The DiFactory allows to register different classes and the interfaces they implement.
    /// There are four different ways the factory can handle classes:
    ///   - Regular Class: Create a new instance each time an instance is required (use registerClass)
    ///   - Singleton Instance: Always use a predefined instance (use registerInstance)
    ///   - Singleton Class on demand: Create an instance the first time it is used and return that instance
    ///     for each request. Once the instance is not needed anymore (by any user), destroy it again.
    ///     (use registerSingleton)
    ///   - Instance Provided At Request: Always use the same instance whenever the specific type is
    ///     requested.
    ///     The actual instance needs to be provided as parameter at requests. If no instance is
    ///     provided, an exception will be thrown.
    ///   - Single Instance Per Request: Similar to Instance Provided At Request, but when no instance
    ///     is provided, a default instance will be created and used whenever needed for this request.
    /// When an instance of an interface is requested, an instance of the class which implements that
    /// interface is returned (this depends on the type of registration of the class).
    ///
    /// The DiFactory will validate that all dependencies can be resolved before an object is created. In
    /// case of an error (missing type, cyclic dependency, ...), an exception is thrown.
    /// To avoid a bigger performance impact, this validation is only done once for each type.
    ///
    /// usage:
    /// \code
    ///   // assume the following classes (and constructors):
    ///
    ///   ClassA: public IntfA1, IntfA2
    ///   {
    ///        ClassA();
    ///   }
    ///
    ///   // used as singleton
    ///   ClassB: public IntfB
    ///   {
    ///        ClassB(const shared_ptr<IntfA1>& intfA1);
    ///
    ///        const shared_ptr<IntfA1> _a1;
    ///   }
    ///
    ///   // used as singleton
    ///   ClassC: public IntfF
    ///   {
    ///        ClassC(int x);
    ///   }
    ///
    ///   // single instance per request (e.g. all dependencies to IntfC
    ///   // should actually point to the same instance)
    ///   ClassD: public IntfD
    ///   {
    ///        ClassC(const shared_ptr<IntfA2>& intfA2);
    ///
    ///        const shared_ptr<IntfA2> _a2;
    ///   }
    ///
    ///   ClassE: public IntfE
    ///   {
    ///        ClassD(const shared_ptr<IntfD>& intfD, const shared_ptr<IntfB>& intfB, const shared_ptr<IntfC>& intfC);
    ///
    ///        const shared_ptr<IntfD> _d;
    ///        const shared_ptr<IntfB> _b;
    ///        const shared_ptr<IntfC> _c;
    ///   }
    ///
    ///   ClassF: public IntfF
    ///   {
    ///        ClassD(const shared_ptr<IntfE>& intfE, const shared_ptr<IntfD>& intfB);
    ///
    ///        const shared_ptr<IntfE> _e;
    ///        const shared_ptr<IntfD> _d;
    ///   }
    ///
    ///   // registration
    ///   diFactory.registerClass<ClassA>().withInterfaces<IntfA1, IntfA2>;
    ///   diFactory.registerSingleton<ClassB, IntfA1>().withInterfaces<MyIntfB>;
    ///   diFactory.registerInstance<ClassC>(make_shared<ClassC>(5)).withInterfaces<IntfB>;
    ///   diFactory.registerInstancePerRequest<ClassD, IntfA2>().withInterfaces<IntfD>;
    ///   diFactory.registerClass<ClassE, IntfD, IntfB, IntfC>().withInterfaces<IntfE>;
    ///   diFactory.registerClass<ClassF, IntfE, IntfB, IntfC>().withInterfaces<IntfF>;
    ///
    ///   //Create two instances of IntfF
    ///   shared_ptr<IntfE> f1 = diFactory.getInstance<IntfF>();
    ///   shared_ptr<IntfF> f2 = diFactory.getInstance<IntfF>();
    ///   // the following members are equal:
    ///   // f1._e._b == f2._e._b
    ///   // f1._e._c == f2._e._c
    ///   // f1._d == f1._e._d
    ///   // f2._d == f2._e._d
    ///   // but f1._d != f2._d
    ///
    ///   weak_ptr<IntfB> b1 = f1._e._b;
    ///   weak_ptr<IntfC> c1 = f1._e._c;
    ///
    ///   // remove all counted references to our singletons
    ///   f1.reset();
    ///   f2.reset();
    ///
    ///   // b1 is expired (not kept alive by the diFactory)
    ///   // c1 is kept alive by the diFactory
    ///
    ///   shared_ptr<IntfF> f3 = diFactory.getInstance<IntfF>();
    ///   shared_ptr<IntfF> f4 = diFactory.getInstance<IntfF>();
    ///   // f3._e._c == f4._e._c == c1 (c1 is kept alive by the diFactory)
    ///   // f3._e._b == f4._e._b
    ///   // but f3._e._b != b1 (b1 is expired)
    /// \endcode
    ///
    class DiFactory
    {

    public:
        /// A helper object which allows to register one or more interfaces
        /// for a specific type.
        /// This object is returned by the various registerXY methods
        /// of the DiFactory.
        template <typename T>
        class InterfaceForType
        {
        public:
            InterfaceForType(DiFactory& diFactory): _diFactory(diFactory) {}

            /// Register the supplied interface types for this object.
            template <typename... I>
            void withInterfaces()
            {
                this->withInterfacesImpl<I...>(NumberToType<sizeof...(I)>());
            }

        private:
            template <unsigned int N> struct NumberToType { };

            template <typename I0, typename... I>
            void withInterfacesImpl(NumberToType<sizeof...(I) + 1>)
            {
                _diFactory.registerInterface<T, I0>();
                this->withInterfacesImpl<I...>(NumberToType<sizeof...(I)>());
            }

            template <typename I>
            void withInterfacesImpl(NumberToType<1>)
            {
                _diFactory.registerInterface<T, I>();
            }

        private:
            DiFactory& _diFactory;
        };

        /// Register a new class and its dependencies.
        /// Getting an instance of this type will internally create a
        /// new instance on the heap and return a shared pointer to it.
        /// \tparam Class  Type of class which should be registered
        /// \tparam Dependencies  List of dependencies of this class.
        ///         The actual values (and order) depends on the parameters
        ///         of the constructor for the specified class.
        template <typename Class, typename... Dependencies>
        InterfaceForType<Class> registerClass()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

             addRegistration<Class>(make_shared<ClassRegistration<Class, Dependencies...> >());
            
            return InterfaceForType<Class>(*this);
        }


        template <typename Class>
        InterfaceForType<Class> registerInstance(shared_ptr<Class> instance)
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            addRegistration<Class>(make_shared<InstanceRegistration<Class> >(instance));

            return InterfaceForType<Class>(*this);
        }


        /// Register a new class as "Single Instance Per Request" and
        /// defines its dependencies.
        /// Getting an instance of this type will internally create a
        /// new instance on the heap and return a shared pointer to it.
        /// If this class is used more than once while resolving the
        /// dependencies, the same instance is used everywhere.
        /// \tparam Class  Type of class which should be registered
        /// \tparam Dependencies  List of dependencies of this class.
        ///         The actual values (and order) depends on the parameters
        ///         of the constructor for the specified class.
        template <typename Class, typename... Dependencies>
        InterfaceForType<Class> registerInstancePerRequest()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            addRegistration<Class>(make_shared<SingleInstancePerRequestRegistration<Class, Dependencies...> >());

            return InterfaceForType<Class>(*this);
        }


        /// Register a new class as "Instance Provided At Request".
        /// Instances of such classes are never created by the
        /// DI-Factory itself. They always need to be provided at
        /// runtime when retrieving an instance.
        /// The same instance will be used whenever such a class is
        /// needed within a request.
        /// A typical use case is a "configuration" class.
        ///
        /// Please note: The validation cannot catch cases where
        /// registerInstanceProvidedAtRequest is used but no
        /// instance is supplied at the actual request.
        /// This means that you either have to provide an instance for
        /// EVERY type registered with registerInstanceProvidedAtRequest
        /// or that you need to know the actual dependencies of the type
        /// that you request.
        /// \tparam Class  Type of class which should be registered
        template <typename Class>
        InterfaceForType<Class> registerInstanceProvidedAtRequest()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            addRegistration<Class>(make_shared<InstanceProvidedAtRequestRegistration<Class> >());

            return InterfaceForType<Class>(*this);
        }


        /// Register a new singleton class and its dependencies.
        /// Getting an instance of this type for the first time
        /// will internally create a new instance on the heap and
        /// return a shared pointer to it.
        /// Once the last shared pointer to the instance is dropped,
        /// the instance will be destroyed again (The factory will
        /// only keep a weak_ptr internally).
        /// \tparam Class  Type of class which should be registered
        /// \tparam Dependencies  List of dependencies of this class.
        ///         The actual values (and order) depends on the parameters
        ///         of the constructor for the specified class.
        /// \note In case a singleton instance should be kept alive
        ///       as long as the application is running, use
        ///       registerInstance instead.
        template <typename Class, typename... Dependencies>
        InterfaceForType<Class> registerSingleton()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            addRegistration<Class>(make_shared<SingletonRegistration<Class, Dependencies...> >());

            return InterfaceForType<Class>(*this);
        }


        /// Register a new interface and defines which class is used
        /// as implementation.
        /// Getting an instance of such an interface will instead
        /// get an instance of that class.
        /// \tparam Class  Type of class which should be registered
        /// \tparam Dependencies  List of dependencies of this class.
        ///         The actual values (and order) depends on the parameters
        ///         of the constructor for the specified class.
        template <typename Class, typename Interface>
        void registerInterface()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            addRegistration<Interface>(make_shared<InterfaceRegistration<Interface, Class> >());
        }


        /// Unregister the specified type.
        template <typename T>
        void unregister()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto it = _registeredTypes.find(type_id<T>());
            if (it != _registeredTypes.end()){
                _registeredTypes.erase(it);
            }

            for (auto itr : _registeredTypes){
                itr.second->invalidate();
            }
        }

        /// Get an instance of the specified type.
        /// According to the registration of this type an existing
        /// singleton or a new instance will be returned.
        /// Before creating an instance, the factory checks if the
        /// registration for this object is complete and valid (This
        /// check will only be done once for each object). If an error
        /// is detected (missing type, cyclic dependency, ...) an
        /// exception will be thrown.
        /// @tparam T         Type which should be return
        /// @tparam Instances Type of instance parameters supplied
        /// @param instances Instance parameters which will be used
        ///                  for the according InstanceProvidedAtRequest and
        ///                  SingleInstancePerRequest types.
        template <typename T, typename... Instances>
        shared_ptr<T> getInstance(const std::shared_ptr<Instances>&... instances)
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };
            AbstractRegistration& registration = findRegistration<T>();
            registration.validate(*this);
            GenericPtrMap typeInstanceMap;

            RegisterInstanceForRequest(typeInstanceMap, instances...);

            return registration.getTypedInstance<T>(*this, typeInstanceMap);
        }


        /// Ensure that there are no registration errors for all types.
        /// The following errors can be detected:
        ///   - Missing types (e.g. dependencies to unregistered types)
        ///   - Cyclic dependencies
        ///   - Dependencies from singletons to "Single Instance Per Request"
        ///     types.
        /// If an error is detected, an exception will be thrown.
        void validate()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            for (auto it: _registeredTypes){
                it.second->validate(*this);
            }
        }

    private:
        friend class AbstractRegistration;

        using GenericPtr    = shared_ptr<void>;
        using GenericPtrMap = unordered_map<size_t, GenericPtr>;

        /// Basic (untyped) class containing registration information
        /// about a specific type.
        /// Each type of registration has it's own Registration class derived from
        /// this class. These derived classes implement the getInstance() behavior
        /// and a validation of the dependencies.
        /// One instance of such a class will be created (and stored) for each registered type.
        class AbstractRegistration
        {
        public:
            virtual ~AbstractRegistration(){}
            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap) = 0;
            virtual void checkAsParam()
            {
                throw new std::logic_error("Not allowed as parameter");
            }

            void validate(const DiFactory& diFactory)
            {
                if (!_validated){
                    isValid(diFactory, this, _hasSiprDependency);
                    _validated = true;
                }
            }

            void validate(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency)
            {
                if (this == root){
                    throw new std::logic_error("circular dependency");
                }

                if (!_validated){
                    isValid(diFactory, root, _hasSiprDependency);
                    _validated = true;
                }

                hasSiprDependency = _hasSiprDependency;
            }

            void invalidate()
            {
                _validated = false;
                _hasSiprDependency = false;
            }

            template <typename T>
            shared_ptr<T> getTypedInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                return static_pointer_cast<T>(getInstance(diFactory, typeInstanceMap));
            }

        protected:
            virtual void isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency) const = 0;

            template <typename T>
            AbstractRegistration& findRegistration(const DiFactory& diFactory) const
            {
                return diFactory.findRegistration<T>();
            }
        private:
            bool _validated;
            bool _hasSiprDependency;
        };

        /// registration for an interface implemented by a specified class
        template <typename Interface, typename Class>
        class InterfaceRegistration: public AbstractRegistration
        {
        public:
            virtual ~InterfaceRegistration(){}
            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                AbstractRegistration& concreteClass = findRegistration<Class>(diFactory);
                return concreteClass.getInstance(diFactory, typeInstanceMap);
            }

        protected:
            virtual void isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency) const
            {
                AbstractRegistration& concreteClass = findRegistration<Class>(diFactory);
                concreteClass.validate(diFactory, root, hasSiprDependency);
            }
        };

        /// registration for regular class created at runtime
        template <typename Class, typename... Dependencies>
        class ClassRegistration: public AbstractRegistration
        {
        public:
            virtual ~ClassRegistration(){}
            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                return make_shared<Class>(getDependencyInstance<Dependencies>(diFactory, typeInstanceMap)...);
            }

        protected:
            virtual void isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency) const
            {
                call(isDependencyValid<Dependencies>(diFactory, root, hasSiprDependency)...);
            }

        private:
            template <typename T>
            shared_ptr<T> getDependencyInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                AbstractRegistration& dependency = findRegistration<T>(diFactory);
                return dependency.getTypedInstance<T>(diFactory, typeInstanceMap);
            }

            template <typename T>
            bool isDependencyValid(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency) const
            {
                AbstractRegistration& dependency = findRegistration<T>(diFactory);
                dependency.validate(diFactory, root, hasSiprDependency);
                return true;
            }

            bool call() const
            {
                return true;
            }

            template <typename T>
            T call(T value) const
            {
                return true;
            }

            template <typename T, typename... Args>
            T call(T first, Args... args) const
            {
                return call(args...);
            }
       };

        /// registration for instance singletons (singleton is kept alive by this object)
        template <typename Class>
        class InstanceRegistration: public AbstractRegistration
        {
        public:
            InstanceRegistration(shared_ptr<Class> instance): _instance(instance) {}
            virtual ~InstanceRegistration(){}

            virtual GenericPtr getInstance(const DiFactory&, GenericPtrMap&)
            {
                return _instance;
            }

        protected:
            virtual void isValid(const DiFactory&, const AbstractRegistration*, bool&) const
            {
                //empty;
            }

        private:
            shared_ptr<Class> _instance;
        };

        /// registration for "weak" singletons (singleton is destroyed when not used any longer)
        template <typename Class, typename... Dependencies>
        class SingletonRegistration: public ClassRegistration<Class, Dependencies...>
        {
        public:
            virtual ~SingletonRegistration(){}

            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                shared_ptr<Class> instance = _instance.lock();
                if (!instance){
                    instance  = static_pointer_cast<Class>(ClassRegistration<Class, Dependencies...>::getInstance(diFactory, typeInstanceMap));
                    _instance = instance;
                }
                return instance;
            }

        protected:
            virtual void isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency) const
            {
                ClassRegistration<Class, Dependencies...>::isValid(diFactory, root, hasSiprDependency);
                if (hasSiprDependency){
                    throw new std::logic_error("Singleton depends on SingleInstancePerRequest class");
                }
            }

        private:
            weak_ptr<Class> _instance;
        };

        /// registration for single instance per request classes
        template <typename Class, typename... Dependencies>
        class SingleInstancePerRequestRegistration: public ClassRegistration<Class, Dependencies...>
        {
        public:
            virtual ~SingleInstancePerRequestRegistration(){}

            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                auto it = typeInstanceMap.find(type_id<Class>());
                if (it != typeInstanceMap.end()){
                    return it->second;
                } else {
                    GenericPtr instance  = ClassRegistration<Class, Dependencies...>::getInstance(diFactory, typeInstanceMap);
                    typeInstanceMap[type_id<Class>()] = instance;
                    return instance;
                }
            }

            virtual void checkAsParam()
            {
                // allowed --> no exception thrown
            }

        protected:
            virtual void isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency) const
            {
                ClassRegistration<Class, Dependencies...>::isValid(diFactory, root, hasSiprDependency);
                hasSiprDependency = true;
            }
        };

        /// registration for single instance per request classes
        template <typename Class>
        class InstanceProvidedAtRequestRegistration: public AbstractRegistration
        {
        public:
            virtual ~InstanceProvidedAtRequestRegistration(){}

            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                auto it = typeInstanceMap.find(type_id<Class>());
                if (it != typeInstanceMap.end()){
                    return it->second;
                } else {
                    throw new std::logic_error("Instance must be supplied at request");
                }
            }

            virtual void checkAsParam()
            {
                // allowed --> no exception thrown
            }

        protected:
            virtual void isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool& hasSiprDependency) const
            {
                // validation is done at design time
            }
        };

        template<typename T>
        AbstractRegistration& findRegistration() const
        {
            const auto it = _registeredTypes.find(type_id<T>());
            if (it != _registeredTypes.end()){
                return *it->second.get();
            } else {
                throw new std::logic_error("type not registered");
            }
        }

        template <typename T>
        void addRegistration(shared_ptr<AbstractRegistration> registration)
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto result = _registeredTypes.insert(std::make_pair(type_id<T>(), registration));

            if (!result.second){
                result.first->second = registration;

                for (auto itr : _registeredTypes){
                    itr.second->invalidate();
                }
            }
        }

        template <typename Instance, typename... Instances>
        void RegisterInstanceForRequest(GenericPtrMap& instanceMap, const std::shared_ptr<Instance>& instance, const std::shared_ptr<Instances>&... instances)
        {
            this->RegisterInstanceForRequest<Instances...>(instanceMap, instance);
            this->RegisterInstanceForRequest<Instances...>(instanceMap, instances...);
        }

        template <typename Instance>
        void RegisterInstanceForRequest(GenericPtrMap& instanceMap, const std::shared_ptr<Instance>& instance)
        {
            AbstractRegistration& registration = findRegistration<Instance>();
            registration.checkAsParam();
            instanceMap[type_id<Instance>()] = instance;
        }

        void RegisterInstanceForRequest(GenericPtrMap& instanceMap)
        {
            //empty
        }

        /// Holds the registration object for the registered types
        unordered_map<size_t, shared_ptr<AbstractRegistration> > _registeredTypes;
        recursive_mutex _mutex;

    };
} // namespace CppDiFactory

#endif // CPP_DI_FACTORY_H
