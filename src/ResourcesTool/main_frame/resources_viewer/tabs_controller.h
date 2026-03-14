#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "Resources/resources.h"
#include <QTabWidget>
#include <QVariantMap>
#include <memory>

class TabsController : public QObject {
	Q_OBJECT
public:
	TabsController(QTabWidget* tabs, Resources* resources);

	void closeByIndex(int index);

	void execute(const QVariantMap& params);
	bool select(const QString& id);

signals:
	void selectWidget(int index, const QString& type, const QString& id);

private slots:
	void onTabChange(int index);

private:
	Resources* _resources;
	QTabWidget* _tabs;
	int _current = 0;
};
