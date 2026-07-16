#pragma once
#include <QObject>
#include <memory>

class Resources;

class CurrentUserContext {
public:
	explicit CurrentUserContext(Resources* resources);
	virtual ~CurrentUserContext();

	void setCurrentUser(const QString& userHash, const QByteArray& encryptionKey);
	void clearCurrentUser();

	QString currentUserHash() const;
	QByteArray currentUserEncryptionKey() const;
	QString currentUserDataDir() const;

	bool isLoggedIn() const;

public:
	static std::pair<QString, QString> generateUserPath(Resources* resources, const QString& userHash);

private:
	class Private;
	std::unique_ptr<Private> d;
};
