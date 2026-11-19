#pragma once
#include "Libs/Engine/ui/doc_widget_stub.h"

#include <memory>

class CommandContext;

/// Документ модуля FetchApi: редактор и выполнение HTTP-запросов.
class FetchApiDocument final : public DocumentWidgetStub {
	Q_OBJECT
public:
	explicit FetchApiDocument(std::shared_ptr<CommandContext> commandContext,
		QWidget* parent = nullptr);
};
