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
    // 1.) Zyklische Abhängigkeit kann noch während der Registrierung überprüft werden

    // DiFactory
    // Dependency injection container aka Inversion of Control (IoC) container.
    //
    // Idea based upon:
    // http://www.codeproject.com/Articles/567981/AnplusIOCplusContainerplususingplusVariadicplusTem
    // https://github.com/Autodesk/goatnative-inject.git


    // Custom type info method that uses size_t and not string for type name -
    // map lookups should go faster than if we would have used RTTI's typeid<T>().name() which returns a string
    // as key.
    // Taken from http://codereview.stackexchange.com/questions/44936/unique-type-id-in-c
    template<typename T>
    struct type { static void id() { } };

    template<typename T>
    size_t type_id() { return reinterpret_cast<size_t>(&type<T>::id); }


    class DiFactory
    {

    public:
        template <typename T>
        class InterfaceForType
        {
        public:
            InterfaceForType(DiFactory& diFactory): _diFactory(diFactory) {}

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


        template <typename Class, typename... Dependencies>
        InterfaceForType<Class> registerClass()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

             _registeredTypes[type_id<Class>()] = make_shared<ClassRegistration<Class, Dependencies...> >();
            
            return InterfaceForType<Class>(*this);
        }


        template <typename Class>
        InterfaceForType<Class> registerInstance(shared_ptr<Class> instance)
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            _registeredTypes[type_id<Class>()] = make_shared<InstanceRegistration<Class> >(instance);

            return InterfaceForType<Class>(*this);
        }


        template <typename Class, typename... Dependencies>
        InterfaceForType<Class> registerInstancePerRequest()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            _registeredTypes[type_id<Class>()] = make_shared<SingleInstancePerRequestRegistration<Class, Dependencies...> >();

            return InterfaceForType<Class>(*this);
        }


        template <typename Class, typename... Dependencies>
        InterfaceForType<Class> registerSingleton()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            _registeredTypes[type_id<Class>()] = make_shared<SingletonRegistration<Class, Dependencies...> >();

            return InterfaceForType<Class>(*this);
        }


        template <typename Class, typename Interface>
        void registerInterface()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            _registeredTypes[type_id<Interface>()] = make_shared<InterfaceRegistration<Interface, Class> >();
        }


        template <typename T>
        void unregister()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto it = _registeredTypes.find(type_id<T>());
            if (it != _registeredTypes.end()){
                _registeredTypes.erase(it);
            }
        }


        template <typename T>
        shared_ptr<T> getInstance()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };
            GenericPtrMap typeInstanceMap;
            return getMyInstance<T>(typeInstanceMap);
        }


        bool isValid()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            bool result = true;
            for (auto it: _registeredTypes){
                result = result && it.second->isValid(*this, nullptr, false);
            }
            return result;
        }

    private:
        friend class AbstractRegistration;

        using GenericPtr    = shared_ptr<void>;
        using GenericPtrMap = unordered_map<size_t, GenericPtr>;

        class AbstractRegistration
        {
        public:
            virtual ~AbstractRegistration(){}
            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap) = 0;
            virtual bool isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool isSingleton) const = 0;

            template <typename T>
            shared_ptr<T> getTypedInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                return static_pointer_cast<T>(getInstance(diFactory, typeInstanceMap));
            }

        protected:
            template <typename T>
            shared_ptr<AbstractRegistration> findRegistration(const DiFactory& diFactory) const
            {
                return diFactory.findRegistration<T>();
            }
        };

        template <typename Interface, typename Class>
        class InterfaceRegistration: public AbstractRegistration
        {
        public:
            virtual ~InterfaceRegistration(){}
            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                shared_ptr<AbstractRegistration> concreteClass = findRegistration<Class>(diFactory);
                if (!concreteClass){
                    throw new std::logic_error("class for interface was not registered");
                }
                return concreteClass->getInstance(diFactory, typeInstanceMap);
            }

            virtual bool isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool isSingleton) const
            {
                shared_ptr<AbstractRegistration> concreteClass = findRegistration<Class>(diFactory);
                return (concreteClass && (this != root) && concreteClass->isValid(diFactory, root ? root : this, isSingleton));
            }
        };

        template <typename Class, typename... Dependencies>
        class ClassRegistration: public AbstractRegistration
        {
        public:
            virtual ~ClassRegistration(){}
            virtual GenericPtr getInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                return make_shared<Class>(getDependencyInstance<Dependencies>(diFactory, typeInstanceMap)...);
            }

            virtual bool isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool isSingleton) const
            {
                return (this != root) && BoolAnd(isDependencyValid<Dependencies>(diFactory, root ? root : this, isSingleton)...);
            }

        private:
            template <typename T>
            shared_ptr<T> getDependencyInstance(const DiFactory& diFactory, GenericPtrMap& typeInstanceMap)
            {
                shared_ptr<AbstractRegistration> dependency = findRegistration<Class>(diFactory);
                if (!dependency){
                    throw new std::logic_error("dependency for class was not registered");
                }
                return dependency->getTypedInstance<T>(diFactory, typeInstanceMap);
            }

            template <typename T>
            bool isDependencyValid(const DiFactory& diFactory, const AbstractRegistration* root, bool isSingleton) const
            {
                shared_ptr<AbstractRegistration> dependency = findRegistration<T>(diFactory);
                return dependency->isValid(diFactory, root, isSingleton);
            }

            bool BoolAnd() const
            {
                return true;
            }

            template <typename T>
            T BoolAnd(T value) const
            {
                return value;
            }

            template <typename T, typename... Args>
            T BoolAnd(T first, Args... args) const
            {
                return first && BoolAnd(args...);
            }
       };

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

            virtual bool isValid(const DiFactory&, const AbstractRegistration*, bool) const
            {
                return true;
            }

        private:
            shared_ptr<Class> _instance;
        };

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

            virtual bool isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool) const
            {
                return ClassRegistration<Class, Dependencies...>::isValid(diFactory, root, true);
            }

        private:
            weak_ptr<Class> _instance;
        };

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

            virtual bool isValid(const DiFactory& diFactory, const AbstractRegistration* root, bool isSingleton) const
            {
                return (!isSingleton) && ClassRegistration<Class, Dependencies...>::isValid(diFactory, root, isSingleton);
            }
        };

        template <typename T>
        shared_ptr<T> getMyInstance(GenericPtrMap& typeInstanceMap)
        {
            shared_ptr<AbstractRegistration> registration = findRegistration<T>();
            if (registration){
                return registration->getTypedInstance<T>(*this, typeInstanceMap);
            } else {
                throw new std::logic_error("Type was not registered");
            }
        }

        template<typename T>
        shared_ptr<AbstractRegistration> findRegistration() const
        {
            const auto it = _registeredTypes.find(type_id<T>());
            if (it != _registeredTypes.end()){
                return it->second;
            } else {
                return shared_ptr<AbstractRegistration>();
            }
        }

        // Holds the registration object for the registered types
        unordered_map<size_t, shared_ptr<AbstractRegistration> > _registeredTypes;
        recursive_mutex _mutex;

    };
} // namespace CppDiFactory

#endif // CPP_DI_FACTORY_H
