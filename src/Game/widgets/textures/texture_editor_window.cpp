#include "Game/widgets/textures/texture_editor_window.h"
#include "Game/widgets/textures/texture_editor_widget.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include "Game/app_controller.h"
#include "Game/commands/command_context.h"
#include "DataLayer/textures/i_textures_data_provider.h"

class TextureEditorWindow::Private {
public:
	Private(TextureEditorWindow* parent) : q(parent) {}
	TextureEditorWindow* q;
	TexturesService* texturesService = nullptr;
	TilesService* tilesService = nullptr;
	TextureEditorWidget* editorWidget = nullptr;
	ApplicationController* controller = nullptr;

	QString selectedTexture;
};

TextureEditorWindow::TextureEditorWindow(
	TexturesService* texturesService,
	TilesService* tilesService,
	const QString& id,
	QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
	d->texturesService = texturesService;
	d->tilesService = tilesService;

	d->editorWidget = new TextureEditorWidget(texturesService, tilesService, this);

	connect(d->editorWidget, &TextureEditorWidget::textureSelected,
		this, &TextureEditorWindow::onTextureSelected);
	connect(d->editorWidget, &TextureEditorWidget::tileSelected,
		this, &TextureEditorWindow::onTileSelected);

	setWidget(d->editorWidget);
}

TextureEditorWindow::~TextureEditorWindow() = default;

bool TextureEditorWindow::handleCommand(const QString& commandName, const QStringList& /*args*/, CommandContext* context) {
	if (commandName == "create") {
		d->controller = context->applicationController();
		return true;
	}
	return false;
}

void TextureEditorWindow::onTextureSelected(TextureType textureType, const QString& fileName) {
	if (d->selectedTexture != fileName) {
		d->selectedTexture = fileName;
		auto context = d->controller->commandContext();
		context->setData("tilemap.tile.id", -1);
		context->setData("tilemap.tile.type", static_cast<int>(textureType));
		context->setData("tilemap.file.name", fileName);
	}
}

void TextureEditorWindow::onTileSelected(int tileId) {
	auto context = d->controller->commandContext();
	context->setData("tilemap.tile.id", tileId);
}
