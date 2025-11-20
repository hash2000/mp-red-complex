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
#include <QVBoxLayout>
#include <memory>
#include <optional>

class StreamWidgetSelector: public QObject {
	Q_OBJECT

public:
	StreamWidgetSelector(std::weak_ptr<Resources> resources,
		QStackedWidget *centerStack,
		QVBoxLayout *actionsLayout);

	void buildStackedView();
	void setSelection(const QStandardItem *item);
	bool isSelected() const;
	std::unique_ptr<QMenu> buildContextMenu(QWidget *parent) const;
	void displayModel();
	void displayHexView();

private:
	QWidget* buildWidget(const QString &suffix, DataStream &stream);
	std::optional<std::reference_wrapper<DataStream>> getStream();
	void cleanViewsData(QWidget *current = nullptr);
	void applyEmptyView();
	void applyHexControls();

private slots:
	void onHexSelectionChanged(qint64 offset, qint64 length, const QByteArray& selected);

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
	QStackedWidget *_centerStack;
	QVBoxLayout *_actionsLayout;
};
