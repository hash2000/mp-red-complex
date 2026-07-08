#pragma once
#include "Launcher/mdi_child_window.h"
#include <QObject>
#include <memory>

class TextureEditorWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit TextureEditorWindow(const QString& id,	QWidget* parent = nullptr);
	~TextureEditorWindow() override;

	QString windowType() const override { return "texture-editor"; }
	QString windowTitle() const override { return "Редактор текстур"; }
	QSize windowDefaultSizes() const override { return QSize(900, 600); }

	bool handleCommand(const std::shared_ptr<Instruction> cmd, CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
