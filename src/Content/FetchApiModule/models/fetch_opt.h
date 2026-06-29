#pragma once
#include <QNetworkRequest>
#include <QByteArray>
#include <QString>

enum class FetchApiActions {
	Undefined,
	Get,
	Post,
	Delete,
	Put,
};

enum class FetchApiSslPolicy {
	Strict,  // All errors processiong
	TrustCustomCa, // Trust only the specified certificates
	IgnoreErrors,
};

class FetchApiOpt {
public:
	QNetworkRequest request;
	FetchApiActions action;
	QString location;
	QByteArray body;
	FetchApiSslPolicy sslPolicy = FetchApiSslPolicy::Strict;
};
