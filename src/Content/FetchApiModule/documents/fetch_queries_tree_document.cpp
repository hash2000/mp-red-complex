#include "Content/FetchApiModule/documents/fetch_queries_tree_document.h"
#include "Content/FetchApiModule/widgets/fetch_queries_tree_widget.h"

FetchQueriesTreeDocument::FetchQueriesTreeDocument(ILazyNodesDataProvider* provider, QWidget* parent)
	: DocumentWidgetStub(new FetchQueriesTreeWidget(provider), parent) {
	setGlyph(QStringLiteral("🌳"));
	setTitle(tr("Запросы Fetch API"));
}
