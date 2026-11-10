#pragma once
#include "Libs/Engine/ui/doc_widget.h"
#include <memory>

class TextDocument : public DocumentWidget {
	Q_OBJECT
public:
	explicit TextDocument(QWidget* parent = nullptr);
	~TextDocument() override;

	bool loadFile(const QString& path);
	QString filePath() const;

	QWidget* createPropertiesEditor(QWidget* parent) override;
	void refresh() override;
	bool save() override;

private:
	struct Private;
	std::unique_ptr<Private> d;   // скрывает DocumentWidget::d (тот private) — это нормально
};
