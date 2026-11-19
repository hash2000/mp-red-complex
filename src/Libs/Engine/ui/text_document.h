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

protected:
	void onContextAttached() override;
	void onContextVariableChanged(const QString& name, const QVariant& value) override;

private:
	void setPath(const QString& path);

	struct Private;
	std::unique_ptr<Private> d;   // скрывает DocumentWidget::d (тот private) — это нормально
};
