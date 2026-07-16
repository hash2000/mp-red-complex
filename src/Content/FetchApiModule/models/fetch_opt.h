#pragma once
#include "Libs/Base/from.h"
#include "Libs/Base/format.h"

#include <QByteArray>
#include <QString>
#include <memory>
#include <map>

class QNetworkRequest;

enum class FetchApiMethod {
	Undefined,
	Get,
	Post,
	Put,
	Delete,
	Patch,
	Head,
};

template <> struct Format<FetchApiMethod> {
	static QString format(const FetchApiMethod& value) {
		switch (value) {
		case FetchApiMethod::Get: return QStringLiteral("get");
		case FetchApiMethod::Post:	return QStringLiteral("post");
		case FetchApiMethod::Put: return ("put");
		case FetchApiMethod::Delete: return QStringLiteral("delete");
		case FetchApiMethod::Patch: return QStringLiteral("patch");
		case FetchApiMethod::Head: return QStringLiteral("head");
		}
		return QStringLiteral("undefined");
	}
};

template <> struct From<FetchApiMethod> {
	static std::optional<FetchApiMethod> from(const QString& value) {
		if (value == "get") return FetchApiMethod::Get;
		else if (value == "post") return FetchApiMethod::Post;
		else if (value == "put") return FetchApiMethod::Put;
		else if (value == "delete") return FetchApiMethod::Delete;
		else if (value == "patch") return FetchApiMethod::Patch;
		else if (value == "head") return FetchApiMethod::Head;
		return std::nullopt;
	}
};


enum class FetchApiSslPolicy {
	Strict,  // All errors processiong
	TrustCustomCa, // Trust only the specified certificates
	IgnoreErrors,
};

class FetchApiOpt {
public:
	FetchApiOpt(
		const QString& location,
		const std::map<QString, QString>& parameters = std::map<QString, QString>(),
		const std::map<QString, QString>& headers = std::map<QString, QString>());

	FetchApiOpt(
		const QString& location,
		const std::map<QString, QString>& parameters = std::map<QString, QString>(),
		const QString& headersFormatted = QString());

	virtual ~FetchApiOpt();

	std::shared_ptr<QNetworkRequest> request() const;

	bool setMethod(FetchApiMethod value);
	bool setMethod(const QString& value);
	FetchApiMethod method() const;

	void setBody(const QByteArray& value);
	const QByteArray body() const;

	void setSslPolicy(FetchApiSslPolicy value);
	FetchApiSslPolicy sslPolicy() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
