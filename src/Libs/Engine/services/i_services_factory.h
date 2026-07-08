#pragma once
#include <memory>
#include <functional>
#include <mutex>

class IServicesFactory {
protected:
	template<typename T>
	class LazyPtr {
		std::unique_ptr<T> _ptr;
		std::once_flag _flag;
		std::function<std::unique_ptr<T>()> _factory;

	public:
		explicit LazyPtr(std::function<std::unique_ptr<T>()> factory)
			: _factory(std::move(factory)) {}

		T* get() {
			std::call_once(_flag, [this] {
				_ptr = _factory();
			});
			return _ptr.get();
		}

		T* operator->() {
			return get();
		}

		T& operator*() {
			return *get();
		}

		bool isInitialized() const noexcept {
			return static_cast<bool>(_ptr);
		}
	};
};
