#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "Resources/resources.h"
#include "DataFormat/proto/msg.h"
#include "DataStream/data_stream.h"
#include <QWidget>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <memory>

class MessagesTableModel;
class MessagesExplorerWidget : public BaseTabWidget {
  Q_OBJECT

public:
  explicit MessagesExplorerWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* parent = nullptr);
  ~MessagesExplorerWidget() override = default;

private:
  QTableView* _tableView;
  MessagesTableModel* _model;
  QSortFilterProxyModel* _proxyModel;
	Proto::Messages::ItemsArray _data;
	std::shared_ptr<DataStream> _stream;
};
