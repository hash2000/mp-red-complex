#pragma once
#include <QString>

struct ActionButtonConfig {
	QString id;              // Уникальный идентификатор кнопки
	QString iconName;        // Название иконки
	QString toolTip;         // Подсказка при наведении
	QString command;         // Команда для выполнения
	int viewOrder = 0;       // Порядок сортировки

	ActionButtonConfig() = default;

	ActionButtonConfig(
		const QString& id,
		const QString& iconName,
		const QString& toolTip,
		const QString& command,
		int viewOrder = 0)
		: id(id)
		, iconName(iconName)
		, toolTip(toolTip)
		, command(command)
		, viewOrder(viewOrder) {
	}
};
