#include "Launcher/widgets/procedure/procedure_explorer_model.h"

ProcedureTableModel::ProcedureTableModel(QObject* parent)
: QAbstractTableModel(parent) {
}

void ProcedureTableModel::setDataRef(const DataFormat::Int::Programmability &data) {
  beginResetModel();
  _data = &data;
  endResetModel();
}

int ProcedureTableModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }

  return _data ? static_cast<int>(_data->procedures.size()) : 0;
}

int ProcedureTableModel::columnCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : Column::Count;
}

QVariant
ProcedureTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
    return {};
  }

  switch (section) {
  case Name:
    return "Name";
  case Flags:
    return "Flags";
  case Delay:
    return "Delay";
  case ConditionOffset:
    return "Cond. Off";
  case BodyOffset:
    return "Body Off";
  case Args:
    return "Args";
  default:
    return {};
  }
}

QVariant ProcedureTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !_data ||
      index.row() >= static_cast<int>(_data->procedures.size())) {
    return {};
  }

  const auto &proc = *_data->procedures[index.row()];

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
    case Name:
      return proc.name;
    case Delay:
      return static_cast<quint32>(proc.delay);
    case ConditionOffset:
      return static_cast<quint32>(proc.conditionOffset);
    case BodyOffset:
      return static_cast<quint32>(proc.bodyOffset);
    case Args:
      return static_cast<quint32>(proc.argumentsCounter);
    case Flags: {
      QStringList parts;
      if (proc.isTimed)
        parts << "Timed";
      if (proc.isConditional)
        parts << "Cond";
      if (proc.isImported)
        parts << "Imp";
      if (proc.isExported)
        parts << "Exp";
      if (proc.isCritical)
        parts << "Crit";
      if (proc.isPure)
        parts << "Pure";
      if (proc.isInline)
        parts << "Inline";
      return parts.join(", ");
    }
    default:
      return {};
    }
  } else if (role == Qt::ToolTipRole && index.column() == Flags) {
    // Подсказка: полный список флагов
    return data(index, Qt::DisplayRole);
  }

  return {};
}
