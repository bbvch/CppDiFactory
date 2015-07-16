//
//  CppDiFactory.h
//
//  Created by Daniel Bernhard and Juergen Messerer on 30.06.2015
//  Copyright (c) 2015 Daniel Bernhard and Juergen Messerer.
//  License: Apache License 2.0
//  All rights reserved.


#ifndef CPP_DI_FACTORY_H
#define CPP_DI_FACTORY_H

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace CppDiFactory
{
    using std::function;
    using std::lock_guard;
    using std::make_shared;
    using std::pair;
    using std::recursive_mutex;
    using std::shared_ptr;
    using std::size_t;
    using std::vector;
    using std::unordered_map;

    //TODO:
	// 1.) Überprüfen, wie das Verhalten bei einer doppel Registrierung sich auswirkt.
	// 2.) Singleton registrieren und erst beim request erzeugen -> lambda shared pointer reference, Singleton evtl als weakptr.
	// 3.) SIPR in Kombination mit Singleton assert werfen
	// 4.) Zyklische Abhängigkeit überprüfen und assert werfen
	// 5.) Deregistrierung



    // DiFactory
    // Dependency injection container aka Inversion of Control (IoC) container.
    //
    // Idea and code based upon:
    // http://www.codeproject.com/Articles/567981/AnplusIOCplusContainerplususingplusVariadicplusTem
    // https://github.com/Autodesk/goatnative-inject.git

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

        template <typename T, typename... Dependencies>
        InterfaceForType<T> registerClass()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto creator = [this](IHolderPtrMap& typeInstanceMap) -> IHolderPtr
            {
                shared_ptr<T> instance = make_shared<T>(getMyInstance<Dependencies>(typeInstanceMap)...);
                return shared_ptr<Holder<T>>{new Holder<T>{ instance }};
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<T>(), creator} );
            
            return InterfaceForType<T>(*this);
        }

        template <typename T>
        InterfaceForType<T> registerInstance(shared_ptr<T> instance)
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            IHolderPtr holder = shared_ptr<Holder<T>>{new Holder<T>{instance}};
            auto creator = [this, holder](IHolderPtrMap& typeInstanceMap) -> IHolderPtr
            {
                return holder;
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<T>(), creator} );

            return InterfaceForType<T>(*this);
        }

        template <typename T, typename... Dependencies>
        InterfaceForType<T> registerInstancePerRequest()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto creator = [this](IHolderPtrMap& typeInstanceMap) -> IHolderPtr
            {
                auto it = typeInstanceMap.find(type_id<T>());
                if (it != typeInstanceMap.end()){
                    return it->second;
                } else {
                    shared_ptr<T> instance = make_shared<T>(getMyInstance<Dependencies>(typeInstanceMap)...);
                    IHolderPtr holder = shared_ptr<Holder<T>>{new Holder<T>{ instance }};
                    typeInstanceMap[type_id<T>()] = holder;
                    return holder;
                }
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<T>(), creator} );

            return InterfaceForType<T>(*this);
        }

        template <typename T, typename... Dependencies>
        InterfaceForType<T> registerSingleton()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto instance = make_shared<T>(getInstance<Dependencies>()...);

            return registerInstance<T>(instance);
        }

        template <typename RegisteredConcreteClass, typename Interface>
        InterfaceForType<RegisteredConcreteClass> registerInterface()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto instanceGetter   = [this](IHolderPtrMap& typeInstanceMap) -> IHolderPtr
            {
                auto instance     = getMyInstance<RegisteredConcreteClass>(typeInstanceMap);
                IHolderPtr holder = shared_ptr<Holder<Interface>>{new Holder<Interface>{ instance }};

                return holder;
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<Interface>(), instanceGetter});

            return InterfaceForType<RegisteredConcreteClass>(*this);
        }

        template <typename T>
        shared_ptr<T> getInstance()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };
            IHolderPtrMap typeInstanceMap;
            return getMyInstance<T>(typeInstanceMap);
        }

    private:
        struct IHolder
        {
            virtual ~IHolder() = default;
        };

        using IHolderPtr    = shared_ptr<IHolder>;
        using IHolderPtrMap = unordered_map<size_t, IHolderPtr>;
        using CreatorLambda = function<IHolderPtr(IHolderPtrMap&)>;

        template <typename T>
        shared_ptr<T> getMyInstance(IHolderPtrMap& typeInstanceMap)
        {
            // Try getting registered singleton or instance.
            auto it =_typesToCreators.find(type_id<T>());
            if (it != _typesToCreators.end())
            {
                auto& creator = it->second;

                auto iholder = creator(typeInstanceMap);
                auto holder  = static_cast<Holder<T>*>(iholder.get());
                return holder->_instance;
            }

            // If you debug, in some debuggers (e.g Apple's lldb in Xcode) it will breakpoint in this assert
            // and by looking in the stack trace you'll be able to see which class you forgot to map.
            assert(false && "One of your injected dependencies isn't mapped, please check your mappings.");

            return nullptr;
        }

        template <typename T>
        struct Holder : public IHolder
        {
            Holder(shared_ptr<T> instance) : _instance(instance) {}

            shared_ptr<T> _instance;
        };

        // Custom type info method that uses size_t and not string for type name -
        // map lookups should go faster than if we would have used RTTI's typeid<T>().name() which returns a string
        // as key.
        // Taken from http://codereview.stackexchange.com/questions/44936/unique-type-id-in-c
        template<typename T>
        struct type { static void id() { } };

        template<typename T>
        size_t type_id() { return reinterpret_cast<size_t>(&type<T>::id); }

        // Holds the lambda function to create a instance of the registered class
        unordered_map<size_t, CreatorLambda> _typesToCreators;

        recursive_mutex _mutex;

    };
} // namespace CppDiFactory

#endif // CPP_DI_FACTORY_H
