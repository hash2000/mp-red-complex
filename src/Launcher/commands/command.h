#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <memory>

class CommandContext; // forward declaration

class CommandAbstraction : public QObject {
public:
	explicit CommandAbstraction(QObject* parent = nullptr) : QObject(parent) {}
	virtual ~CommandAbstraction() = default;

	// Выполнение команды
	virtual bool execute(CommandContext* context, const QStringList& args) = 0;

	// Название команды (для вызова из консоли)
	virtual QString name() const = 0;

	// Краткое описание
	virtual QString description() const = 0;

	// Подробная справка (синтаксис, примеры)
	virtual QString help() const;

	// Минимальное количество аргументов (без учёта имени команды)
	virtual int minArgs() const;

	// Максимальное количество аргументов (-1 = без ограничения)
	virtual int maxArgs() const;

	static QString parseArgsValue(const QStringList& args, const QString& name);
};
