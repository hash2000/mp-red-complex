#pragma once

#include "Game/mdi_child_window.h"
#include <QObject>
#include <memory>

class CodeEditorWindow : public MdiChildWindow {
	Q_OBJECT
public:
	explicit CodeEditorWindow(const QString& id, QWidget* parent = nullptr);
	~CodeEditorWindow() override;

	QString windowType() const override { return "code-editor"; }
	QString windowTitle() const override { return "Editor"; }
	QSize windowDefaultSizes() const override { return QSize(800, 600); }
	QString help() const { return "code-editor path:<path>"; }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;


private slots:
	void onChangeTargetPath();
	void onOpenDocumentClick();
	void onSaveDocumentClick();

private:
	class Private;
	std::unique_ptr<Private> d;
};
