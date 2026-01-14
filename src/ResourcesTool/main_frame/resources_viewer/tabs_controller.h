#pragma once
#include "ResourcesTool/widgets/base_tab_widget.h"
#include "Resources/resources.h"
#include <QTabWidget>
#include <memory>

class TabsController : public QObject {
	Q_OBJECT
public:
	TabsController(QTabWidget* tabs, std::shared_ptr<Resources> resources);

	void closeByIndex(int index);

	void add(const QString& type, const QString& id);
	bool contains(const QString& id);

signals:
	void selectWidget(int index, const QString& type, const QString& id);

private slots:
	void onTabChange(int index);

private:
	std::shared_ptr<Resources> _resources;
	QTabWidget* _tabs;
	int _current = 0;
};
