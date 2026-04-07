#include "Game/widgets/textures/texture_editor_window.h"
#include "Game/widgets/textures/texture_editor_widget.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "Game/app_controller.h"
#include "Game/commands/command_context.h"

class TextureEditorWindow::Private {
public:
	Private(TextureEditorWindow* parent) : q(parent) {}
	TextureEditorWindow* q;
	TexturesService* texturesService = nullptr;
	TextureEditorWidget* editorWidget = nullptr;
	ApplicationController* controller = nullptr;
};

TextureEditorWindow::TextureEditorWindow(TexturesService* texturesService, const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
	d->texturesService = texturesService;

	d->editorWidget = new TextureEditorWidget(texturesService, this);

	setWidget(d->editorWidget);
	resize(windowDefaultSizes());
}

TextureEditorWindow::~TextureEditorWindow() = default;

bool TextureEditorWindow::handleCommand(const QString& commandName, const QStringList& /*args*/, CommandContext* context) {
	if (commandName == "create") {
		d->controller = context->applicationController();
		return true;
	}
	return false;
}
