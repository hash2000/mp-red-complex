#pragma once
#include <QObject>
#include <QVariant>
#include <QMetaType>
#include <typeinfo>
#include <QDebug>

class EventBus : public QObject {
	Q_OBJECT

public:
	explicit EventBus(QObject* parent = nullptr);
	~EventBus() override;

	/**
	 * Публикация события любого типа.
	 * Требует регистрации типа через qRegisterMetaType<T>()
	 */
	template<typename T>
	void publish(const T& event) {
		// Проверка регистрации типа (для отладки)
		if (!QMetaType::hasRegisteredConverterFunction<T, QVariant>()) {
			qWarning()
				<< "Event type not registered:" << typeid(T).name()
				<< "Call qRegisterMetaType<" << typeid(T).name()
				<< ">() in main()";
		}

		emit eventPublished(QVariant::fromValue(event));
	}

	/**
	 * Подписка на событие конкретного типа.
	 * Возвращает подключённый сигнал для удобного управления соединениями.
	 */
	template<typename T, typename Functor>
	QMetaObject::Connection subscribe(Functor&& handler) {
		return connect(this, &EventBus::eventPublished, this,
			[handler = std::forward<Functor>(handler)](const QVariant& variant) {
				if (variant.canConvert<T>()) {
					T event = variant.value<T>();
					handler(event);
				}
			}, Qt::DirectConnection);
	}

signals:
	void eventPublished(const QVariant& event);
};
