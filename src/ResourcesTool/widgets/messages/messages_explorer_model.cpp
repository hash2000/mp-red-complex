#include "ResourcesTool/widgets/messages/messages_explorer_model.h"

MessagesTableModel::MessagesTableModel(QObject* parent)
: QAbstractTableModel(parent) {
}

void MessagesTableModel::setDataRef(const Proto::Messages::ItemsArray &data) {
  beginResetModel();
  _data = &data;
  endResetModel();
}

int MessagesTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return _data ? static_cast<int>(_data->size()) : 0;
}

int MessagesTableModel::columnCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : Column::Count;
}

QVariant MessagesTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return {};
  }

  switch (section) {
  case Identity:
    return "Id";
  case Context:
    return "Context";
  case Message:
    return "Msessage";
  default:
    return {};
  }
}

QVariant MessagesTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !_data ||
      index.row() >= static_cast<int>(_data->size())) {
    return {};
  }

	const auto &data = *_data;
  const auto &item = data[index.row()];

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case Identity:
      return item.id;
    case Context:
      return item.context;
    case Message:
      return item.text;
    default:
      return {};
    }
  }

  return {};
}
