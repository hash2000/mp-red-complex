#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <memory>

class CommandContext;
class Instruction;

class CommandAbstraction : public QObject {
public:
	explicit CommandAbstraction(QObject* parent = nullptr) : QObject(parent) {}
	virtual ~CommandAbstraction() = default;

	// Выполнение команды
	virtual bool execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) = 0;

	// Название команды (для вызова из консоли)
	virtual QString name() const = 0;

	// Краткое описание
	virtual QString description() const = 0;

	// Подробная справка (синтаксис, примеры)
	virtual QString help() const {
		return description();
	}

	// Минимальное количество аргументов (без учёта имени команды)
	virtual int minArgs() const {
		return 0;
	}

	// Максимальное количество аргументов (-1 = без ограничения)
	virtual int maxArgs() const {
		return -1;
	}
};
