#pragma once
#include "Libs/Engine/ui/doc_widget.h"
#include <memory>

class CodeEditorWidget;
class HighlightingPluginManager;
class FormatterPluginManager;

/// Документ редактора кода: лента управления (файл, шрифт, язык,
/// форматирование) сверху, CodeEditorWidget — под ней. Лента сворачивается
/// небольшой кнопкой в правом углу.
///
/// Обёртка хранит путь к файлу и состояние изменённости, поэтому
/// приложение корректно реагирует на изменение документа.
class CodeEditorDocument : public DocumentWidget {
	Q_OBJECT
public:
	explicit CodeEditorDocument(
		HighlightingPluginManager* highlighting,
		FormatterPluginManager* formatters,
		QWidget* parent = nullptr);
	~CodeEditorDocument() override;

	CodeEditorWidget* editor() const;
	QString filePath() const;

	/// Создать пустой документ (кнопка «Новый файл»).
	void newFile();
	/// Загрузить файл в редактор (чтение выполняется асинхронно).
	bool openFile(const QString& path);

	QWidget* createPropertiesEditor(QWidget* parent) override;
	void refresh() override;
	bool save() override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
