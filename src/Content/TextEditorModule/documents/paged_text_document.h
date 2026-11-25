#pragma once
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <memory>

class FormatterPluginManager;
class HighlightingPluginManager;
class PagedTextEditor;

/// Документ-обёртка над виртуальным редактором больших файлов.
/// Путь берётся из переменной контекста `path`; без пути это документ
/// «Без имени» с полноценным редактируемым буфером. Документ умеет
/// редактировать текст, вставлять из буфера обмена, отменять правки и
/// сохранять файл: изменённые страницы держатся в памяти (при превышении
/// предела выгружаются во временный файл), неизменённые читаются из файла.
class PagedTextDocument : public DocumentWidget {
	Q_OBJECT
public:
	explicit PagedTextDocument(QWidget* parent = nullptr);
	PagedTextDocument(HighlightingPluginManager* highlighting,
		FormatterPluginManager* formatters, QWidget* parent = nullptr);
	~PagedTextDocument() override;

	PagedTextEditor* editor() const;

	QWidget* createPropertiesEditor(QWidget* parent) override;
	QList<DocumentPropertyGroup> createPropertyGroups(QWidget* parent) override;
	void refresh() override;
	bool save() override;

	/// Открыть файл и записать путь в контекст документа.
	void openFile(const QString& path);
	void newFile();

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
