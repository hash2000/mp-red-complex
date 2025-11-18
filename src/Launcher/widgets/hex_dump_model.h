#pragma once
#include <QAbstractTableModel>

class HexDumpModel : public QAbstractTableModel {
  Q_OBJECT

public:
  explicit HexDumpModel(QObject *parent = nullptr);

  void setData(const QByteArray &data);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;

  int columnCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
  QByteArray _data;
  static constexpr int BYTES_PER_ROW = 16;
};
