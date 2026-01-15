#pragma once
#include "Resources/resources/model/assets_model.h"
#include "Resources/resources.h"
#include "DataStream/data_stream.h"
#include <QString>
#include <QVariantMap>
#include <QStandardItem>
#include <memory>
#include <optional>

class HexControlPanel;

class StreamWidgetSelector: public QObject {
	Q_OBJECT

public:
	StreamWidgetSelector();

	void setSelection(const QStandardItem *item);
	bool isSelected() const;
	void execute();
	void displayHexView(const QByteArray& data);
	std::optional<std::shared_ptr<DataStream>> getStream(Resources& resources) const;

	AssetsViewItemType getType() const;
	QString getSuffix() const;
	QString getContainerName() const;
	QString getContainerPath() const;
	QVariantMap getSelection() const;

signals:
	void beforeStreamSelection(const QVariantMap& params);

private:
	struct {
		QString container;
		QString path;
		QString suffix;
		AssetsViewItemType type;
	} _selection;
};
