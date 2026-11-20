#pragma once
#include "Content/DatabaseModule/data_providers/i_data_blobs_provider.h"

#include <QObject>
#include <QString>

#include <memory>
#include <optional>

/// Сервис блоков данных SystemRouter.
///
/// Блоки хранят крупные значения свойств узлов (скрипты, многострочный текст,
/// JSON) и доступны code-editor по GUID.
class SystemRouterBlobService : public QObject {
	Q_OBJECT
public:
	explicit SystemRouterBlobService(IDataBlobsProvider* provider, QObject* parent = nullptr);
	~SystemRouterBlobService() override;

	IDataBlobsProvider* dataProvider() const;

	QList<DataBlob> blobs() const;
	std::optional<DataBlob> blob(const QString& id) const;
	QString data(const QString& id) const;

	/// Сохранить содержимое блока; метаданные существующего блока сохраняются.
	bool saveData(const QString& id, const QString& data);

	/// Создать новый блок и вернуть его GUID (пустая строка при ошибке).
	QString createData(const QString& data);

	bool removeBlob(const QString& id);

	QString lastError() const;

signals:
	void errorOccurred(const QString& message);

private:
	class Private;
	std::unique_ptr<Private> d;
};
