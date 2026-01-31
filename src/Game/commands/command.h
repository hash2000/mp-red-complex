#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <memory>

class CommandContext; // forward declaration

class ICommand : public QObject {
public:
	explicit ICommand(QObject* parent = nullptr) : QObject(parent) {}
	virtual ~ICommand() = default;

	// Выполнение команды
	virtual bool execute(CommandContext* context, const QStringList& args) = 0;

	// Название команды (для вызова из консоли)
	virtual QString name() const = 0;

	// Краткое описание
	virtual QString description() const = 0;

	// Подробная справка (синтаксис, примеры)
	virtual QString help() const { return description(); }

	// Минимальное количество аргументов (без учёта имени команды)
	virtual int minArgs() const { return 0; }

	// Максимальное количество аргументов (-1 = без ограничения)
	virtual int maxArgs() const { return -1; }
};
