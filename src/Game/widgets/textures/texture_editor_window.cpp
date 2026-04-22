#include "Game/widgets/textures/texture_editor_window.h"
#include "Game/widgets/textures/texture_editor_widget.h"
#include "ApplicationLayer/textures/images_service.h"
#include "ApplicationLayer/textures/tiles_service.h"
#include "Game/app_controller.h"
#include "Game/commands/command_context.h"
#include "DataLayer/images/i_images_data_provider.h"

class TextureEditorWindow::Private {
public:
	Private(TextureEditorWindow* parent) : q(parent) {}
	TextureEditorWindow* q;
	ImagesService* ImagesService = nullptr;
	TilesService* tilesService = nullptr;
	TextureEditorWidget* editorWidget = nullptr;
	ApplicationController* controller = nullptr;
};

TextureEditorWindow::TextureEditorWindow(
	ImagesService* ImagesService,
	TilesService* tilesService,
	const QString& id,
	QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
	d->ImagesService = ImagesService;
	d->tilesService = tilesService;

	d->editorWidget = new TextureEditorWidget(ImagesService, tilesService, this);

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
