#include "Launcher/widgets/textures/texture_editor_window.h"
#include "Launcher/widgets/textures/texture_editor_widget.h"
#include "ApplicationLayer/textures/images_service.h"
#include "ApplicationLayer/textures/tiles_selector_service.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/commands/command_context.h"
#include "DataLayer/images/i_images_data_provider.h"

class TextureEditorWindow::Private {
public:
	Private(TextureEditorWindow* parent) : q(parent) {}
	TextureEditorWindow* q;
};

TextureEditorWindow::TextureEditorWindow(const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
}

TextureEditorWindow::~TextureEditorWindow() = default;

bool TextureEditorWindow::handleCommand(const QString& commandName, const QStringList& /*args*/, CommandContext* context) {
	if (commandName == "create") {
		auto services = context->services();
		auto editorWidget = new TextureEditorWidget(
			services->imagesService(),
			services->tilesSelectorService(), this);

		setWidget(editorWidget);

		return true;
	}
	return false;
}
