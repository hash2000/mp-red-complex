#include "Launcher/widgets/messages/messages_explorer_widget.h"
#include "Launcher/widgets/messages/messages_explorer_model.h"
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QApplication>
#include <QVBoxLayout>


MessagesExplorerWidget::MessagesExplorerWidget(
	std::unique_ptr<Proto::Messages> data,
	std::shared_ptr<DataStream> stream,
	QWidget* parent)
  : QWidget(parent)
	, _stream(stream)
  , _model(new MessagesTableModel(this))
  , _proxyModel(new QSortFilterProxyModel(this))
{
  if (!data) {
    qWarning() << "MessagesExplorerWidget: received nullptr data";
  }

	_data = Proto::to_array(data->items);
	_model->setDataRef(_data);

  _tableView = new QTableView(this);
  _proxyModel->setSourceModel(_model);
  _tableView->setModel(_proxyModel);
  _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  _tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  _tableView->setAlternatingRowColors(true);
  _tableView->setSortingEnabled(true);

	auto header = _tableView->horizontalHeader();
	header->setStretchLastSection(true);
	header->setSectionResizeMode(MessagesTableModel::Identity, QHeaderView::Interactive);
	for (int i = 1; i < header->count(); i++) {
		header->setSectionResizeMode(i, QHeaderView::Interactive);
		header->resizeSection(i, 25);
	}

	_tableView->setColumnWidth(MessagesTableModel::Message,
			std::max(200, header->sectionSize(MessagesTableModel::Message)));

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(_tableView);
  setLayout(layout);
}



