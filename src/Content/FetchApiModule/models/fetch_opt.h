#pragma once
#include "Libs/Base/from.h"
#include "Libs/Base/format.h"

#include <QNetworkRequest>
#include <QByteArray>
#include <QString>

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
	QNetworkRequest request;
	FetchApiMethod method;
	QByteArray body;
	FetchApiSslPolicy sslPolicy = FetchApiSslPolicy::Strict;
};
