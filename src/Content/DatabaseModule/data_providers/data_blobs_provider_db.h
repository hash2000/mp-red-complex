#pragma once
#include "Content/DatabaseModule/data_providers/i_data_blobs_provider.h"

#include <QString>
#include <memory>

class DatabasesService;

/// Поставщик блоков данных в SQLite.
///
/// Ожидаемая структура таблицы:
///   id          TEXT PRIMARY KEY (GUID)
///   version     INTEGER NOT NULL
///   description TEXT NOT NULL
///   data        TEXT NOT NULL
class DataBlobsProviderDb final : public IDataBlobsProvider {
public:
	DataBlobsProviderDb(DatabasesService* databasesService, const QString& databaseName,
		const QString& tableName = QStringLiteral("data_blobs"));
	~DataBlobsProviderDb() override;

	QList<DataBlob> blobs() const override;
	std::optional<DataBlob> blob(const QString& id) const override;
	bool saveBlob(const DataBlob& blob) override;
	bool removeBlob(const QString& id) override;
	QString lastError() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
