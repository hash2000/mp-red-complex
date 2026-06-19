#include "Launcher/commands/command.h"
#include <QRegularExpression>

// Подробная справка (синтаксис, примеры)
QString CommandAbstraction::help() const {
	return description();
}

// Минимальное количество аргументов (без учёта имени команды)
int CommandAbstraction::minArgs() const {
	return 0;
}

// Максимальное количество аргументов (-1 = без ограничения)
int CommandAbstraction::maxArgs() const {
	return -1;
}

QString CommandAbstraction::parseArgsValue(const QStringList& args, const QString& name) {
	const auto expr = QString("^%1:").arg(name);
	return args.filter(QRegularExpression(expr))
		.value(0)
		.mid(name.length() + 1);
}
