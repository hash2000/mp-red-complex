#include "Launcher/widgets/procedure/procedure_explorer_widget.h"
#include "Launcher/widgets/procedure/procedure_explorer_model.h"
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QApplication>
#include <QVBoxLayout>


ProcedureExplorerWidget::ProcedureExplorerWidget(std::unique_ptr<Format::Int::Programmability> data,
	QWidget* parent)
  : QWidget(parent)
	, _data(std::move(data))
  , _model(new ProcedureTableModel(this))
  , _proxyModel(new QSortFilterProxyModel(this)) {

  if (!_data) {
    qWarning() << "ProcedureExplorerWidget: received nullptr Programmability";
  }

	 _model->setDataRef(*_data);

  _tableView = new QTableView(this);
  _proxyModel->setSourceModel(_model);
  _tableView->setModel(_proxyModel);

  // Настройки отображения
  _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  _tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  _tableView->setAlternatingRowColors(true);
  _tableView->setSortingEnabled(true);
  // _tableView->horizontalHeader()->setStretchLastSection(true);
  // _tableView->horizontalHeader()->setSectionResizeMode(ProcedureTableModel::Name, QHeaderView::Stretch);

	auto header = _tableView->horizontalHeader();


	header->setStretchLastSection(true);
	header->setSectionResizeMode(ProcedureTableModel::Name, QHeaderView::Interactive);
	for (int i = 1; i < header->count(); i++) {
		header->setSectionResizeMode(i, QHeaderView::Interactive);
		header->resizeSection(i, 25);
	}

	// _tableView->resizeColumnsToContents();
	_tableView->setColumnWidth(ProcedureTableModel::Name,
			std::max(150, header->sectionSize(ProcedureTableModel::Name)));

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(_tableView);
  setLayout(layout);
}

ProcedureExplorerWidget::~ProcedureExplorerWidget() = default;

// void ProcedureExplorerWidget::setProgrammability(const Format::Int::Programmability& data) {
//   _model->setDataRef(data);
//   _tableView->resizeColumnsToContents();

// 	if (_tableView->columnWidth(ProcedureTableModel::Name) < 150) {
//     _tableView->setColumnWidth(ProcedureTableModel::Name, 150);
// 	}
// }

