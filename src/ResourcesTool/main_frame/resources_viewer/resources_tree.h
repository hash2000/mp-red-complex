#pragma once
#include <QTreeView>
#include <QMimeData>

class ResourcesTree: public QTreeView {
		Q_OBJECT

public:
		explicit ResourcesTree(QWidget * parent = nullptr);

protected:
		void startDrag(Qt::DropActions supportedActions) override;
};
