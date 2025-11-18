#pragma once
#include "Launcher/widgets/hex_dump_widget.h"
#include "Resources/resources/model/assets_model.h"
#include "Resources/resources.h"
#include "DataStream/data_stream.h"
#include <QString>
#include <QStandardItem>
#include <QStackedWidget>
#include <QWidget>
#include <QMenu>
#include <memory>
#include <optional>

class StreamWidgetSelector {
public:
	StreamWidgetSelector(std::weak_ptr<Resources> resources);

	void buildStackedView(QStackedWidget *view);

	void setSelection(const QStandardItem *item);

	bool isSelected() const;

	std::unique_ptr<QMenu> buildContextMenu(QWidget *parent) const;

	void displayModel(QStackedWidget *view);

	void displayHexView(QStackedWidget *view);

private:
	QWidget* buildWidget(const QString &suffix, DataStream &stream) const;

	std::optional<std::reference_wrapper<DataStream>> getStream();

	void clenViewsData(QWidget *current = nullptr);

	void applyEmptyView(QStackedWidget *view);

private:
	struct {
		QWidget *empty;
		HexDumpWidget *hex;
	} _views;

	struct {
		QString container;
		QString path;
		QString suffix;
		AssetsViewItemType type;
	} _selection;

	std::weak_ptr<Resources> _resources;
};
