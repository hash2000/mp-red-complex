#pragma once
#include <typeindex>
#include <any>
#include <memory>
#include <functional>
#include <unordered_map>

class IServiceHolder {
public:
	virtual ~IServiceHolder() = default;
};

template <typename T>
class ServiceHolder : public IServiceHolder {
public:
	std::unique_ptr<T> service;

	explicit ServiceHolder(std::unique_ptr<T> s) : service(std::move(s)) {}
};

class ServicesRegistry {
public:
	ServicesRegistry() = default;
	~ServicesRegistry() = default;

	template<typename T>
	T* get() {
		auto type = std::type_index(typeid(T));

		auto factoryIt = _factories.find(type);
		if (factoryIt != _factories.end()) {
			auto serviceIt = _services.find(type);
			if (serviceIt == _services.end()) {
				factoryIt->second();
			}
		}

		auto it = _services.find(type);
		if (it != _services.end()) {
			auto* holder = static_cast<ServiceHolder<T>*>(it->second.get());
			return holder->service.get();
		}

		return nullptr;
	}

protected:
	template<typename T>
	void registerService(std::unique_ptr<T> service) {
		_services[std::type_index(typeid(T))] = std::make_unique<ServiceHolder<T>>(std::move(service));
	}

	template<typename T, typename Factory>
	void registerFactory(Factory&& factory) {
		_factories[std::type_index(typeid(T))] = [this, factory = std::forward<Factory>(factory)]() mutable {
			if (_services.find(std::type_index(typeid(T))) == _services.end()) {
				auto service = factory();
				registerService<T>(std::move(service));
			}
		};
	}

private:
	std::unordered_map<std::type_index, std::function<void()>> _factories;
	std::unordered_map<std::type_index, std::unique_ptr<IServiceHolder>> _services;
};
