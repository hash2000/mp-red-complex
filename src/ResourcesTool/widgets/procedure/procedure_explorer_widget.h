#pragma once
#include "ResourcesTool/widgets/base_tab_widget.h"
#include "DataFormat/proto/procedure.h"
#include "DataStream/data_stream.h"
#include <QWidget>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <memory>

class ProcedureTableModel;
class ProcedureExplorerWidget : public BaseTabWidget {
  Q_OBJECT

public:
  explicit ProcedureExplorerWidget(
		std::unique_ptr<Proto::Programmability> data,
		std::shared_ptr<DataStream> stream,
		QWidget* parent = nullptr);
  ~ProcedureExplorerWidget() override = default;

signals:
	void selectProcedure(
		DataStream &stream,
		Proto::Programmability &prog,
		Proto::Procedure &proc);

private slots:
	void onProcDoubleClick(const QModelIndex &index);

private:
  QTableView* _tableView;
  ProcedureTableModel* _model;
  QSortFilterProxyModel* _proxyModel;
	std::unique_ptr<Proto::Programmability> _data;
	std::shared_ptr<DataStream> _stream;
};
