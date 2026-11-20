#pragma once
#include <QString>

/// Блок данных: крупное значение свойства или скрипт команды.
///
/// Хранится в таблице `data_blobs`, чтобы кавычки и переносы строк не попадали
/// в JSON-описание свойств узла.
struct DataBlob {
	QString id;          ///< GUID блока
	int version = 1;     ///< версия обработчика/данных
	QString description; ///< описание
	QString data;        ///< содержимое

	bool isValid() const {
		return !id.trimmed().isEmpty();
	}
};
