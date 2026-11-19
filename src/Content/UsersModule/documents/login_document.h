#pragma once
#include "Libs/Engine/ui/doc_widget_stub.h"

class UsersService;

/// Документ формы входа.
class LoginDocument final : public DocumentWidgetStub {
	Q_OBJECT
public:
	explicit LoginDocument(UsersService* usersService, QWidget* parent = nullptr);
};
