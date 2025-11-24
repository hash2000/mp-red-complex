#pragma once
#include "Game/data_format/int/procedure.h"
#include <QAbstractTableModel>


class ProcedureTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    Name = 0,
    Flags,
    Delay,
    ConditionOffset,
    BodyOffset,
    Args,
    Count
  };

  explicit ProcedureTableModel(QObject* parent = nullptr);

public:
  void setDataRef(const Format::Int::Programmability& data);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
  const Format::Int::Programmability* _data = nullptr;
};
