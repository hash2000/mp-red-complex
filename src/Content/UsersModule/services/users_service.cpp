#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/users/i_users_data_provider.h"
#include "Content/UsersModule/models/user_view.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/TexturesModule/data_providers/i_images_data_provider.h"

#include <QCryptographicHash>
#include <QUuid>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
//#include <sodium.h>

class UsersService::Private {
public:
	Private(UsersService* parent) : q(parent) { }
	UsersService* q;

	IUsersDataProvider* usersDataProvider = nullptr;
	ImagesService* imagesService = nullptr;
	Resources* resources;
};

UsersService::UsersService(
	Resources* resources,
	IUsersDataProvider* usersDataProvider,
	ImagesService* imagesService,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->usersDataProvider = usersDataProvider;
	d->imagesService = imagesService;
}

UsersService::~UsersService() = default;

std::optional<QString> UsersService::login(const QString& login, const QString& password) {
	if (isAuthenticated()) {
		logout();
	}

	return std::nullopt;
}

void UsersService::logout() {
	emit loggedOut();
}

bool UsersService::isAuthenticated() const {
	return false;
}

