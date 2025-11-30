#pragma once
#include "Game/data_format/msg/msg.h"
#include <QAbstractTableModel>


class MessagesTableModel : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    Identity = 0,
    Context,
    Message,
    Count
  };

  explicit MessagesTableModel(QObject* parent = nullptr);

public:
  void setDataRef(const DataFormat::Msg::Messages::ItemsArray& data);
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
  const DataFormat::Msg::Messages::ItemsArray* _data = nullptr;
};
