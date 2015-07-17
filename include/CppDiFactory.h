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
    using std::static_pointer_cast;
    using std::vector;
    using std::weak_ptr;
    using std::unordered_map;

    //TODO:
	// 1.) Überprüfen, wie das Verhalten bei einer doppel Registrierung sich auswirkt.
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

            auto creator = [this](GenericPtrMap& typeInstanceMap) -> GenericPtr
            {
                return make_shared<T>(getMyInstance<Dependencies>(typeInstanceMap)...);
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<T>(), creator} );
            
            return InterfaceForType<T>(*this);
        }

        template <typename T>
        InterfaceForType<T> registerInstance(shared_ptr<T> instance)
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            GenericPtr holder(instance);
            auto creator = [this, holder](GenericPtrMap& typeInstanceMap) -> GenericPtr
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

            auto creator = [this](GenericPtrMap& typeInstanceMap) -> GenericPtr
            {
                auto it = typeInstanceMap.find(type_id<T>());
                if (it != typeInstanceMap.end()){
                    return it->second;
                } else {
                    shared_ptr<T> instance = make_shared<T>(getMyInstance<Dependencies>(typeInstanceMap)...);
                    typeInstanceMap[type_id<T>()] = instance;
                    return instance;
                }
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<T>(), creator} );

            return InterfaceForType<T>(*this);
        }

        template <typename T, typename... Dependencies>
        InterfaceForType<T> registerSingleton()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            weak_ptr<T> holder;
            auto creator = [this, holder](GenericPtrMap& typeInstanceMap) mutable -> GenericPtr
            {
                shared_ptr<T> instance = holder.lock();
                if(!instance){
                    instance =  make_shared<T>(getMyInstance<Dependencies>(typeInstanceMap)...);
                    holder = instance;
                }

                return instance;
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<T>(), creator} );

            return InterfaceForType<T>(*this);
        }

        template <typename RegisteredConcreteClass, typename Interface>
        InterfaceForType<RegisteredConcreteClass> registerInterface()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };

            auto instanceGetter   = [this](GenericPtrMap& typeInstanceMap) -> GenericPtr
            {
                return getMyInstance<RegisteredConcreteClass>(typeInstanceMap);
            };

            _typesToCreators.insert(pair<size_t, CreatorLambda>{type_id<Interface>(), instanceGetter});

            return InterfaceForType<RegisteredConcreteClass>(*this);
        }

        template <typename T>
        shared_ptr<T> getInstance()
        {
            lock_guard<recursive_mutex> lockGuard{ _mutex };
            GenericPtrMap typeInstanceMap;
            return getMyInstance<T>(typeInstanceMap);
        }

    private:
        using GenericPtr    = shared_ptr<void>;
        using GenericPtrMap = unordered_map<size_t, GenericPtr>;
        using CreatorLambda = function<GenericPtr(GenericPtrMap&)>;

        template <typename T>
        shared_ptr<T> getMyInstance(GenericPtrMap& typeInstanceMap)
        {
            // Try getting registered singleton or instance.
            auto it =_typesToCreators.find(type_id<T>());
            if (it != _typesToCreators.end())
            {
                auto& creator = it->second;
                // Convert a shared_ptr<void> to the needed shared_ptr<T>.
                // It's possible to use static_pointer_cast since it's
                // guaranteed that the returned instance is of the correct
                // type.
                return static_pointer_cast<T>(creator(typeInstanceMap));
            }

            // If you debug, in some debuggers (e.g Apple's lldb in Xcode) it will breakpoint in this assert
            // and by looking in the stack trace you'll be able to see which class you forgot to map.
            assert(false && "One of your injected dependencies isn't mapped, please check your mappings.");

            return nullptr;
        }

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
