#include "ResourcesTool/widgets/procedure/procedure_explorer_widget.h"
#include "ResourcesTool/widgets/procedure/procedure_explorer_model.h"
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QApplication>
#include <QVBoxLayout>


ProcedureExplorerWidget::ProcedureExplorerWidget(
	std::unique_ptr<Proto::Programmability> data,
	std::shared_ptr<DataStream> stream,
	QWidget* parent)
  : BaseTabWidget(parent)
	, _data(std::move(data))
	, _stream(stream)
  , _model(new ProcedureTableModel(this))
  , _proxyModel(new QSortFilterProxyModel(this))
{
  if (!_data) {
    qWarning() << "ProcedureExplorerWidget: received nullptr Programmability";
  }

	 _model->setDataRef(*_data);

  _tableView = new QTableView(this);
  _proxyModel->setSourceModel(_model);
  _tableView->setModel(_proxyModel);
  _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  _tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  _tableView->setAlternatingRowColors(true);
  _tableView->setSortingEnabled(true);

	auto header = _tableView->horizontalHeader();
	header->setStretchLastSection(true);
	header->setSectionResizeMode(ProcedureTableModel::Name, QHeaderView::Interactive);
	for (int i = 1; i < header->count(); i++) {
		header->setSectionResizeMode(i, QHeaderView::Interactive);
		header->resizeSection(i, 25);
	}

	_tableView->setColumnWidth(ProcedureTableModel::Name,
			std::max(150, header->sectionSize(ProcedureTableModel::Name)));

	connect(_tableView, &QTableView::doubleClicked,
		this, &ProcedureExplorerWidget::onProcDoubleClick);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(_tableView);
  setLayout(layout);
}

void ProcedureExplorerWidget::onProcDoubleClick(const QModelIndex &index) {
	if (!index.isValid()) {
		return;
	}

	const auto sourceIndex = _proxyModel->mapToSource(index);
	if (!sourceIndex.isValid()) {
		return;
	}

	const auto name = _model->data(sourceIndex, ProcedureTableModel::Column::Name);
	for(auto &it : _data->procedures) {
		if (it->name == name) {
			emit selectProcedure(*_stream, *_data, *it);
			return;
		}
	}
}



