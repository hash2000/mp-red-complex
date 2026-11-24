#pragma once
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <memory>

/// Документ-обёртка над виртуальным редактором больших файлов.
/// Прототип: только просмотр, путь берётся из переменной контекста `path`.
class PagedTextDocument : public DocumentWidget {
	Q_OBJECT
public:
	explicit PagedTextDocument(QWidget* parent = nullptr);
	~PagedTextDocument() override;

	QWidget* createPropertiesEditor(QWidget* parent) override;

	/// Открыть файл и записать путь в контекст документа.
	void openFile(const QString& path);

protected:
	void onContextAttached() override;
	void onContextVariableChanged(const QString& name, const QVariant& value) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};

/// Метаданные типа: путь к файлу; он же — подсказка вкладки.
class PagedTextDocumentMetadata final : public IDocumentMetadata {
public:
	QList<DocumentVariable> variables() const override;
	QString tooltipVariable() const override;
};
