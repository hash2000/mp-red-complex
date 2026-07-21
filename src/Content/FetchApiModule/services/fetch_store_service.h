#pragma once
#include <QObject>
#include <memory>

class IFetchQueriesDataProvider;
class IFetchTreeNodesDataProvider;

class FetchStoreService : public QObject {
public:
	FetchStoreService(
		IFetchQueriesDataProvider* queriesDataProvider,
		IFetchTreeNodesDataProvider* treeNodesDataProvider,
		QObject* parent = nullptr);

	~FetchStoreService() override;


private:
	class Private;
	std::unique_ptr<Private> d;
};
