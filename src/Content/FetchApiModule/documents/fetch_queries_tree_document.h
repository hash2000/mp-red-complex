#pragma once
#include "Libs/Engine/ui/doc_widget_stub.h"

class ILazyNodesDataProvider;

/// Документ дерева запросов модуля FetchApi.
class FetchQueriesTreeDocument final : public DocumentWidgetStub {
	Q_OBJECT
public:
	explicit FetchQueriesTreeDocument(ILazyNodesDataProvider* provider,
		QWidget* parent = nullptr);
};
