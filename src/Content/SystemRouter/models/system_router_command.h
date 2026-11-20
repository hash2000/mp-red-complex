#pragma once
#include <QString>
#include <QStringList>

/// Команда маршрутизатора: запись таблицы `commands`.
struct SystemRouterCommand {
	QString id;          ///< GUID команды (без фигурных скобок)
	int version = 1;     ///< версия обработчика
	QString description; ///< описание скрипта
	QString script;      ///< текст скрипта с блоками {{параметр}}

	bool isValid() const {
		return !id.trimmed().isEmpty();
	}
};

/// Подготовленный к выполнению скрипт узла.
struct SystemRouterScriptPreview {
	QString propertyName;          ///< имя свойства-команды (название скрипта)
	QString commandId;             ///< GUID из значения свойства
	SystemRouterCommand command;   ///< запись таблицы команд
	bool commandFound = false;     ///< команда найдена в таблице
	QString renderedScript;        ///< скрипт с подставленными параметрами
	QStringList missingParameters; ///< параметры, для которых нет значений
	QString error;                 ///< причина, по которой скрипт нельзя выполнить
};

/// Приводит GUID команды к каноническому виду (без фигурных скобок).
/// Не-GUID строка возвращается без изменений — решение о допустимости
/// принимает поставщик/сервис.
QString normalizeSystemRouterCommandId(const QString& raw);
