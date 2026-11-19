#pragma once
#include "Libs/Engine/ui/doc_widget_stub.h"

class UsersService;

/// Документ профиля пользователя.
class UserDocument final : public DocumentWidgetStub {
	Q_OBJECT
public:
	explicit UserDocument(UsersService* usersService, QWidget* parent = nullptr);
};
