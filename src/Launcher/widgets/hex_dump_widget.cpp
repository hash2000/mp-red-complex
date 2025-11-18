#include "Launcher/widgets/hex_dump_widget.h"
#include <QFontDatabase>
#include <QFont>
#include <QVBoxLayout>
#include <QHeaderView>

HexDumpWidget::HexDumpWidget(QWidget *parent)
: QWidget(parent)
, _tableView(new QTableView(this))
, _model(new HexDumpModel(this))
{
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);  // убираем отступы
  layout->addWidget(_tableView);
  setLayout(layout);

  _tableView->setModel(_model);
  _tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  _tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  _tableView->setShowGrid(false);
	//_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  //_tableView->horizontalHeader()->hide();
  //_tableView->verticalHeader()->hide();

  for (int i = 0; i < 16; ++i) {
      _tableView->setColumnWidth(i, 10);
  }
  _tableView->setColumnWidth(16, 120);
	_tableView->setStyleSheet("QTableView::item { padding: 0px 2px; }");

  connect(_tableView->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          this,
          &HexDumpWidget::onSelectionChanged);

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPointSize(10);
  setFont(font);
}

void HexDumpWidget::setByteArray(const QByteArray &data) {
	_data = data;
	_model->setData(_data);
}

QByteArray HexDumpWidget::byteArray() const {
  return _data;
}

void HexDumpWidget::clear() {
	if (_data.isEmpty()) {
		return;
	}

	_data.clear();
	_data.squeeze();
}

QByteArray HexDumpWidget::selectRange(qint64 offset, qint64 length) {
  if (_data.isEmpty() || offset < 0 || length <= 0) {
    return {};
  }

  const qint64 maxOffset = static_cast<qint64>(_data.size());
  if (offset >= maxOffset) {
    return {};
  }

  length = qMin(length, maxOffset - offset);

  setSelectionRange(offset, length);

  scrollToByte(offset);

  return _data.mid(static_cast<int>(offset), static_cast<int>(length));
}

void HexDumpWidget::setSelectionRange(qint64 offset, qint64 length) {
  if (!_tableView || !_model) {
    return;
  }

  // Сбрасываем старое выделение
  _tableView->selectionModel()->clearSelection();

  // Рассчитываем ячейки: 16 байт на строку, 2 столбца на байт? или 1 столбец = 1 байт?
  // Предположим: 16 байт/строка, 1 столбец = 1 байт (колонки 0..15 — hex, 16 — ASCII)
  static constexpr int BYTES_PER_ROW = 16;

  const int startRow = static_cast<int>(offset / BYTES_PER_ROW);
  const int startCol = static_cast<int>(offset % BYTES_PER_ROW);

  const qint64 endOffset = offset + length - 1;
  const int endRow = static_cast<int>(endOffset / BYTES_PER_ROW);
  const int endCol = static_cast<int>(endOffset % BYTES_PER_ROW);

  // Создаём выделение
  QItemSelection selection;
  for (int row = startRow; row <= endRow; ++row) {
    int colStart = (row == startRow) ? startCol : 0;
    int colEnd = (row == endRow) ? endCol : (BYTES_PER_ROW - 1);

    QModelIndex topLeft = _model->index(row, colStart);
    QModelIndex bottomRight = _model->index(row, colEnd);

    if (topLeft.isValid() && bottomRight.isValid()) {
      selection.merge(QItemSelection(topLeft, bottomRight), QItemSelectionModel::Select);
    }
  }

  _tableView->selectionModel()->select(selection, QItemSelectionModel::Select);
  _tableView->setFocus();

  // Сохраняем для selectedRange()
  _selectionOffset = offset;
  _selectionLength = length;
}

void HexDumpWidget::scrollToByte(qint64 offset) {
  if (!_tableView || !_model) {
    return;
  }

  static constexpr int BYTES_PER_ROW = 16;
  const int row = static_cast<int>(offset / BYTES_PER_ROW);

  QModelIndex index = _model->index(row, 0);
  if (index.isValid()) {
    _tableView->scrollTo(index, QAbstractItemView::PositionAtTop);
  }
}

QPair<qint64, qint64> HexDumpWidget::selectedRange() const {
  return {_selectionOffset, _selectionLength};
}

void HexDumpWidget::onSelectionChanged() {
  if (!_tableView || !_model) {
    return;
  }

  const QItemSelectionModel *selModel = _tableView->selectionModel();
  const QModelIndexList indexes = selModel->selectedIndexes();

  if (indexes.isEmpty()) {
    _selectionOffset = -1;
    _selectionLength = 0;
    emit selectionChanged(_selectionOffset, _selectionLength);  // ← см. ниже про сигнал
    return;
  }

  // Находим минимальный и максимальный byte-индекс в выделении
  static constexpr int BYTES_PER_ROW = 16;

  qint64 minByte = std::numeric_limits<qint64>::max();
  qint64 maxByte = -1;

  for (const QModelIndex &index : indexes) {
    if (index.column() >= BYTES_PER_ROW) {
      continue;  // пропускаем ASCII-колонку (если она у тебя отдельно)
    }

    const qint64 byteIndex = static_cast<qint64>(index.row()) * BYTES_PER_ROW + index.column();
    minByte = qMin(minByte, byteIndex);
    maxByte = qMax(maxByte, byteIndex);
  }

  if (minByte <= maxByte && minByte < _data.size()) {
    _selectionOffset = minByte;
    _selectionLength = maxByte - minByte + 1;

    // Ограничиваем длину доступными данными
    if (_selectionOffset + _selectionLength > _data.size()) {
      _selectionLength = _data.size() - _selectionOffset;
    }
  } else {
    _selectionOffset = -1;
    _selectionLength = 0;
  }

  emit selectionChanged(_selectionOffset, _selectionLength);
}
