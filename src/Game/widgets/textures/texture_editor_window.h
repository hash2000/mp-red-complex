#pragma once
#include "Game/mdi_child_window.h"
#include <QObject>
#include <memory>

class TexturesService;
class TilesService;
class TextureEditorWidget;
enum class ImageType;

class TextureEditorWindow : public MdiChildWindow {
	Q_OBJECT

public:
	explicit TextureEditorWindow(
		TexturesService* texturesService,
		TilesService* tilesService,
		const QString& id,
		QWidget* parent = nullptr);
	~TextureEditorWindow() override;

	QString windowType() const override { return "texture-editor"; }
	QString windowTitle() const override { return "Редактор текстур"; }
	QSize windowDefaultSizes() const override { return QSize(900, 600); }

	bool handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
