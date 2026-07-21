#pragma once
#include <QString>

#include <optional>
#include <list>

class FetchTree;
class FetchQuery;

class IFetchTreeNodesDataProvider {
public:
	virtual ~IFetchTreeNodesDataProvider() = default;

	virtual std::list<FetchTree> treeNodes(std::optional<int> parentId) = 0;
	virtual bool deleteTreeNode(int id) = 0;
	virtual bool updateTreeNode(const FetchTree& node) = 0;
};
