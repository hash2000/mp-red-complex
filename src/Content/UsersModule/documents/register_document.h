#pragma once
#include "Libs/Engine/ui/doc_widget_stub.h"

class UsersRegistryService;

/// Документ формы регистрации нового пользователя.
class RegisterDocument final : public DocumentWidgetStub {
	Q_OBJECT
public:
	explicit RegisterDocument(UsersRegistryService* usersRegistryService,
		QWidget* parent = nullptr);
};
