#pragma once
#include <QString>

#include <optional>
#include <list>

class FetchTree {
public:
	int id;
	std::optional<int> parentId;
	QString name;

	std::optional<std::list<FetchTree>> children;
};
