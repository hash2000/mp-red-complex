#include "Launcher/widgets/hex_dump_model.h"
#include "hex_dump_model.h"

HexDumpModel::HexDumpModel(QObject *parent) {
}

int HexDumpModel::rowCount(const QModelIndex &) const {
  if (_data.isEmpty()) {
    return 0;
  }
  return (_data.size() + BYTES_PER_ROW - 1) / BYTES_PER_ROW;
}

int HexDumpModel::columnCount(const QModelIndex &) const {
  return BYTES_PER_ROW + 1;  // +1 для ASCII-колонки
}

QVariant HexDumpModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || role != Qt::DisplayRole) {
    return {};
  }

  const int row = index.row();
  const int col = index.column();
  const int byteIndex = row * BYTES_PER_ROW + col;

  if (col < BYTES_PER_ROW) {
    if (byteIndex < _data.size()) {
      return QString("%1")
				.arg(static_cast<quint8>(_data[byteIndex]), 2, 16, QChar('0'))
				.toUpper();
    }
    return QString("  ");
  } else if (col == BYTES_PER_ROW) {
    // ASCII колонка
    QString ascii;
    for (int i = 0; i < BYTES_PER_ROW; ++i) {
      int idx = row * BYTES_PER_ROW + i;
      if (idx < _data.size()) {
        char c = _data[idx];
        ascii += (c >= 32 && c <= 126) ? QChar(c) : '.';
      } else {
        ascii += ' ';
      }
    }
    return ascii;
  }

  return {};
}

void HexDumpModel::setData(const QByteArray &data) {
  beginResetModel();
  _data = data;
  endResetModel();
}
