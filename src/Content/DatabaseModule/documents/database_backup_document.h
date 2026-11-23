#pragma once
#include "Libs/Engine/ui/doc_widget.h"

#include <memory>

class DatabaseBackupService;
class DatabaseBackupWidget;

/// Документ «Резервное копирование баз данных».
///
/// Оборачивает `DatabaseBackupWidget`: виджет выполняет операции с файлами,
/// документ хранит выбранное хранилище в переменной контекста `store` и
/// показывает сведения о нём в панели «Свойства». Список хранилищ берётся из
/// зарегистрированных контекстов `.store.<alias>`.
class DatabaseBackupDocument final : public DocumentWidget {
	Q_OBJECT
public:
	explicit DatabaseBackupDocument(DatabaseBackupService* service, QWidget* parent = nullptr);
	~DatabaseBackupDocument() override;

	DatabaseBackupWidget* widget() const;

	void refresh() override;
	bool save() override;
	QWidget* createPropertiesEditor(QWidget* parent) override;

protected:
	void onContextAttached() override;
	void onContextVariableChanged(const QString& name, const QVariant& value) override;

private:
	void printMessage(const QString& message, bool error);

	struct Private;
	std::unique_ptr<Private> d;
};
