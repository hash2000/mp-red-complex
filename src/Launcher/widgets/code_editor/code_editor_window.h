#pragma once

#include "Launcher/mdi_child_window.h"
#include <QObject>
#include <memory>

class CodeEditorWindow : public MdiChildWindow {
	Q_OBJECT
public:
	explicit CodeEditorWindow(const QString& id, QWidget* parent = nullptr);
	~CodeEditorWindow() override;

	QString windowType() const override { return "code-editor"; }
	QString windowTitle() const override;
	QSize windowDefaultSizes() const override { return QSize(800, 600); }
	QString help() const;

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private slots:
	void onOpenDocumentClick();
	void onSaveDocumentClick();

private:
	class Private;
	std::unique_ptr<Private> d;
};
