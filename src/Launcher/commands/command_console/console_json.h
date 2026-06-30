#pragma once
#include <QString>
#include <QJsonDocument>
#include <QByteArray>
#include <memory>

class ConsoleJson {
public:
	ConsoleJson();
	explicit ConsoleJson(const QString& jsonString);
	explicit ConsoleJson(const QJsonDocument& jsonDoc);
	explicit ConsoleJson(const QByteArray& jsonData);
	~ConsoleJson();

	bool load(const QString& jsonString);
	bool load(const QJsonDocument& jsonDoc);
	bool load(const QByteArray& jsonData);

	QString toHtml() const;
	QString toHtml(const QString& styleClass) const;

	bool isValid() const;
	QString getErrorMessage() const;
	bool isEmpty() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};

