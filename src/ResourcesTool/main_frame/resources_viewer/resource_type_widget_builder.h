#pragma once
#include "ResourcesTool/main_frame/resources_viewer/widget_resources.h"
#include "ResourcesTool/main_frame/resources_viewer/stream_widget_selector.h"
#include "DataFormat/proto/procedure.h"
#include "Resources/resources.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <memory>

class ResourceTypeWidgetBuilder: public QObject {
	Q_OBJECT
public:
	ResourceTypeWidgetBuilder(
		std::shared_ptr<StreamWidgetSelector> selector,
		std::shared_ptr<Resources> resources,
		QTabWidget *centerTabs);

	void build(const QString &suffix);
	bool selectTabByName(const QString &name);

private:
	void tryBuild(const QString &suffix);

private slots:
	void onSelectProcedure(
		DataStream &stream,
		Proto::Programmability &prog,
		Proto::Procedure &proc);

private:
	void clearLayout(QVBoxLayout *layout);
	void addWidgetTab(WidgetResource type, std::shared_ptr<DataStream> block, QWidget *widget);

	void makeHex(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeText(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeInt(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeMsg(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeSve(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeBio(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeGcd(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeGam(WidgetResource type, std::shared_ptr<DataStream> block);
	void makePro(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeFrm(WidgetResource type, std::shared_ptr<DataStream> block, const QString &suffix);
	void makePal(WidgetResource type, std::shared_ptr<DataStream> block);
	void makeMap(WidgetResource type, std::shared_ptr<DataStream> block);

private:
	std::shared_ptr<StreamWidgetSelector> _selector;
	std::shared_ptr<Resources> _resources;
	QTabWidget *_centerTabs;
	//QVBoxLayout *_actionsLayout;
};
