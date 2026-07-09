#include "Launcher/windows/textures/texture_editor_window.h"
#include "Content/TexturesModule/widgets/texture_editor_widget.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/TexturesModule/services/tiles_selector_service.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Engine/services/services_registry.h"

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

bool TextureEditorWindow::handleCommand(const std::shared_ptr<Instruction> cmd, CommandContext* context) {
	const auto action = cmd->parameters.value("action");
	if (!action.isNull() && action == "create") {
		auto services = context->services();
		auto editorWidget = new TextureEditorWidget(
			services->get<ImagesService>(),
			services->get<TilesSelectorService>(), this);

		setWidget(editorWidget);

		return true;
	}
	return false;
}
