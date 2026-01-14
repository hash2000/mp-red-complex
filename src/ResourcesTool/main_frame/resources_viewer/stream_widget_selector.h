#pragma once
#include "Resources/resources/model/assets_model.h"
#include "Resources/resources.h"
#include "DataStream/data_stream.h"
#include <QString>
#include <QStandardItem>
#include <QStackedWidget>
#include <QWidget>
#include <QMenu>
#include <QVBoxLayout>
#include <memory>
#include <optional>
#include <QPlainTextEdit>

class HexControlPanel;

class StreamWidgetSelector: public QObject {
	Q_OBJECT

public:
	StreamWidgetSelector();

	void setSelection(const QStandardItem *item);
	bool isSelected() const;
	void displayModel(Resources& resources);
	void displayHexView(const QByteArray& data);
	std::optional<std::shared_ptr<DataStream>> getStream(Resources& resources) const;

	AssetsViewItemType getType() const;
	QString getSuffix() const;

signals:
	void beforeStreamSelection(const QString& suffix, std::optional<std::shared_ptr<DataStream>> stream);

private:
	struct {
		QString container;
		QString path;
		QString suffix;
		AssetsViewItemType type;
	} _selection;
};
