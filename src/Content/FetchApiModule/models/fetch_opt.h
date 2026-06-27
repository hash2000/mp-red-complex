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

class FetchApiOpt {
public:
	QNetworkRequest request;
	FetchApiActions action;
	QString location;
	QByteArray body;
};
