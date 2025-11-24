#pragma once
#include "Game/data_format/int/procedure.h"
#include <QWidget>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <memory>

class ProcedureTableModel;
class ProcedureExplorerWidget : public QWidget {
  Q_OBJECT

public:
  explicit ProcedureExplorerWidget(std::unique_ptr<Format::Int::Programmability> data,
		QWidget* parent = nullptr);
  ~ProcedureExplorerWidget() override;

private:
  QTableView* _tableView;
  ProcedureTableModel* _model;
  QSortFilterProxyModel* _proxyModel;
	std::unique_ptr<Format::Int::Programmability> _data;
};
