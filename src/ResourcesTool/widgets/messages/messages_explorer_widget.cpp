#include "ResourcesTool/widgets/messages/messages_explorer_widget.h"
#include "ResourcesTool/widgets/messages/messages_explorer_model.h"
#include "DataFormat/data_format/msg/data_reader.h"
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QApplication>
#include <QVBoxLayout>


MessagesExplorerWidget::MessagesExplorerWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
	QWidget* parent)
: BaseTabWidget(resources, params, parent)
  , _model(new MessagesTableModel(this))
  , _proxyModel(new QSortFilterProxyModel(this))
{
	auto block = currentStream();
	auto result = std::make_unique<Proto::Messages>();
	DataFormat::Msg::DataReader reader(*block);
	reader.read(*result);

  if (!result) {
    qWarning() << "MessagesExplorerWidget: received nullptr data";
  }

	_data = Proto::to_array(result->items);
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



