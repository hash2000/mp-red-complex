#pragma once
#include <QObject>
#include <memory>

class Resources;

class RegisterUserService : public QObject {
	Q_OBJECT
public:
	explicit RegisterUserService(Resources* resources, QObject* parent = nullptr);

	~RegisterUserService() override;

	bool registerUser(
		const QString& login,
		const QString& password,
		const QString& displayName);

private:
	class Private;
	std::unique_ptr<Private> d;
};
