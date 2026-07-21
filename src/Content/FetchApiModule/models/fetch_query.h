#pragma once
#include <QString>

#include <optional>

class FetchQuery {
public:
	int tree_id;
	QString method;
	QString request;
	std::optional<QString> parameters;
	std::optional<QString> headers;
	std::optional<QString> body;
};
