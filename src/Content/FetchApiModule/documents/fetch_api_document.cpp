#include "Content/FetchApiModule/documents/fetch_api_document.h"
#include "Content/FetchApiModule/widgets/fetch_api_widget.h"

FetchApiDocument::FetchApiDocument(std::shared_ptr<CommandContext> commandContext, QWidget* parent)
	: DocumentWidgetStub(new FetchApiWidget(std::move(commandContext)), parent) {
	setGlyph(QStringLiteral("🌐"));
	setTitle(tr("Fetch API"));
}
