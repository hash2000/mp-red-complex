#pragma once
#include "Content/DatabaseModule/models/data_blob.h"

#include <QList>
#include <QString>

#include <optional>

/// Поставщик блоков данных (таблица `data_blobs`).
class IDataBlobsProvider {
public:
	virtual ~IDataBlobsProvider() = default;

	virtual QList<DataBlob> blobs() const = 0;
	virtual std::optional<DataBlob> blob(const QString& id) const = 0;
	virtual bool saveBlob(const DataBlob& blob) = 0;
	virtual bool removeBlob(const QString& id) = 0;
	virtual QString lastError() const = 0;
};
