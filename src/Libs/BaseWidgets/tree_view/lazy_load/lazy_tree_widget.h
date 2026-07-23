#pragma once
#include <QWidget>

class ILazyNodesDataProvider;

class LazyTreeViewWidget : public QWidget {
	Q_OBJECT
public:
	explicit LazyTreeViewWidget(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~LazyTreeViewWidget() override;

private slots:
	void onSearchTextChanged(const QString& text);
	void performSearch();
	void onAddNode();
	void onDeleteNode();
	void onBackToNormalView();
	void showContextMenu(const QPoint& pos);

private:
	class Private;
	std::unique_ptr<Private> d;
};
