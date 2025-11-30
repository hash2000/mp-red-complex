#pragma once
#include "Game/data_format/msg/msg.h"
#include "DataStream/data_stream.h"
#include <QWidget>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <memory>

class MessagesTableModel;
class MessagesExplorerWidget : public QWidget {
  Q_OBJECT

public:
  explicit MessagesExplorerWidget(
		std::unique_ptr<DataFormat::Msg::Messages> data,
		std::shared_ptr<DataStream> stream,
		QWidget* parent = nullptr);
  ~MessagesExplorerWidget() override = default;

private:
  QTableView* _tableView;
  MessagesTableModel* _model;
  QSortFilterProxyModel* _proxyModel;
	DataFormat::Msg::Messages::ItemsArray _data;
	std::shared_ptr<DataStream> _stream;
};
