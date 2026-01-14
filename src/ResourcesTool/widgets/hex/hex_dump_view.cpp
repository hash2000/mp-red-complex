#include "ResourcesTool/widgets/hex/hex_dump_view.h"
#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QPainter>
#include <QScreen>
#include <QScrollBar>
#include <QMouseEvent>

HexDumpView::HexDumpView(QWidget* parent)
	: QAbstractScrollArea(parent) {
	setObjectName("HexDumpView");
	setFocusPolicy(Qt::StrongFocus);
	setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	_duplicateUpdateTimer.setSingleShot(true);
	_duplicateUpdateTimer.setInterval(200);

	connect(&_duplicateUpdateTimer, &QTimer::timeout, this,
		&HexDumpView::updateDuplicateMap);

	updateMetrics();
	verticalScrollBar()->setSingleStep(_metrics.rowHeight);
	verticalScrollBar()->setPageStep(_metrics.rowHeight * 20);
}

void HexDumpView::setData(const QByteArray& data) {
	_data = data;
	_selStart = _selEnd = -1;

	const int rows =
		(_data.size() + _metrics.bytesPerRow - 1) / _metrics.bytesPerRow;
	const int contentHeight = rows * _metrics.rowHeight;

	verticalScrollBar()->setRange(0,
		qMax(0, contentHeight - viewport()->height()));
	viewport()->update();

	_duplicateUpdateTimer.start();
}

qsizetype HexDumpView::getDataSize() const {
	return _data.size();
}

void HexDumpView::clear() {
	_data.clear();
	_data.squeeze();
	_selStart = _selEnd = -1;
	_byteFrequency.clear();
	verticalScrollBar()->setRange(0, 0);
	viewport()->update();
}

void HexDumpView::selectRange(qint64 offset, qint64 length) {
	if (_data.isEmpty() || offset < 0 || length <= 0) {
		_selStart = _selEnd = -1;
	}
	else {
		length = qMin(length, static_cast<qint64>(_data.size()) - offset);
		_selStart = offset;
		_selEnd = offset + length - 1;
	}
	emitSelectionChange();
	viewport()->update();

	if (_selStart >= 0) {
		scrollToByte(_selStart);
	}
}

void HexDumpView::scrollToByte(qint64 offset, bool ensureVisible /* = true */) {
	if (_data.isEmpty() || offset < 0) {
		return;
	}
	const int row = static_cast<int>(offset / _metrics.bytesPerRow);
	const int targetY = row * _metrics.rowHeight;

	if (!ensureVisible) {
		verticalScrollBar()->setValue(targetY);
		return;
	}

	const int viewTop = verticalScrollBar()->value();
	const int viewHeight = viewport()->height();
	const int viewBottom = viewTop + viewHeight;

	// Проверяем, попадает ли targetY в [viewTop, viewBottom)
	// Но лучше — чтобы вся *строка* была видна, не только её верхний край:
	const int targetBottom = targetY + _metrics.rowHeight;

	const bool isFullyVisible = (targetY >= viewTop) && (targetBottom <= viewBottom);
	const bool isPartiallyVisible = (targetY < viewBottom) && (targetBottom > viewTop);

	// Если хотя бы частично видна — не прокручиваем (или можно использовать isFullyVisible)
	if (isFullyVisible) {
		return;
	}

	int idealTop = targetY - (viewHeight - _metrics.rowHeight) / 4;
	idealTop = qBound(0, idealTop, verticalScrollBar()->maximum());

	verticalScrollBar()->setValue(idealTop);
}

QByteArray HexDumpView::selectedData() const {
	if (_selStart < 0 || _selEnd < 0) {
		return {};
	}
	const qint64 start = qMin(_selStart, _selEnd);
	const qint64 end = qMax(_selStart, _selEnd);
	const qint64 len =
		qMin(end - start + 1, static_cast<qint64>(_data.size() - start));
	return _data.mid(static_cast<int>(start), static_cast<int>(len));
}

QPair<qint64, qint64> HexDumpView::selectedRange() const {
	if (_selStart < 0) {
		return { -1, 0 };
	}
	const qint64 start = qMin(_selStart, _selEnd);
	const qint64 end = qMax(_selStart, _selEnd);
	return { start, end - start + 1 };
}

void HexDumpView::setHighlightDuplicates(bool enabled) {
	if (_highlightDuplicates != enabled) {
		_highlightDuplicates = enabled;
		if (enabled) {
			_duplicateUpdateTimer.start();
		}
		else {
			_byteFrequency.clear();
		}
		viewport()->update();
	}
}

void HexDumpView::setBytesPerRow(int count) {
	if (count > 0 && count <= 64) {
		_metrics.bytesPerRow = count;
		updateMetrics();
		setData(_data); // пересчитать высоту и перерисовать
	}
}

void HexDumpView::setFontSize(int pt) {
	QFont f = font();
	f.setPointSize(pt);
	setFont(f);
	updateMetrics();
	setData(_data);
}

void HexDumpView::updateMetrics() {
	QFont f = font();
	// Учёт DPI/scaling (Qt6+)
	if (QScreen* screen = QApplication::primaryScreen()) {
		const qreal dpiRatio = screen->devicePixelRatio();
		f.setPointSizeF(f.pointSizeF() * dpiRatio);
	}

	QFontMetrics fm(f);
	_font = f;
	_metrics.charWidth = fm.horizontalAdvance('0');
	_metrics.rowHeight = fm.height() + 4; // + вертикальный отступ

	// Ширины:
	_metrics.offsetWidth = fm.horizontalAdvance("FFFFFFFF ") + 4;
	const int hexColWidth = _metrics.charWidth * 2 + 4; // "FF "
	_metrics.hexStartX = _metrics.offsetWidth;
	_metrics.asciiStartX =
		_metrics.hexStartX + _metrics.bytesPerRow * hexColWidth + 10;

	viewport()->update();
}

void HexDumpView::updateDuplicateMap() {
	if (!_highlightDuplicates || _data.isEmpty()) {
		_byteFrequency.clear();
		return;
	}

	_byteFrequency.clear();
	for (char c : _data) {
		++_byteFrequency[static_cast<quint8>(c)];
	}
	viewport()->update();
}

qint64 HexDumpView::posToByte(const QPoint& pos) const {
	const int row = (pos.y() + verticalScrollBar()->value()) / _metrics.rowHeight;
	const int relX = pos.x() - _metrics.hexStartX;
	const int col = relX / (_metrics.charWidth * 2 + 4);

	if (row < 0 || col < 0 || col >= _metrics.bytesPerRow) {
		return -1;
	}

	const qint64 idx = static_cast<qint64>(row) * _metrics.bytesPerRow + col;
	return (idx < _data.size()) ? idx : -1;
}

void HexDumpView::emitSelectionChange() {
	if (_selStart >= 0) {
		const qint64 start = qMin(_selStart, _selEnd);
		const qint64 end = qMax(_selStart, _selEnd);
		const qint64 len =
			qMin(end - start + 1, static_cast<qint64>(_data.size() - start));
		emit selectionChanged(start, len, selectedData());
	}
	else {
		emit selectionChanged(-1, 0, QByteArray());
	}
}

void HexDumpView::paintEvent(QPaintEvent* event) {
	QPainter p(viewport());
	p.setFont(_font);
	p.setRenderHint(QPainter::Antialiasing, false);

	const int scrollY = verticalScrollBar()->value();
	const int firstRow = scrollY / _metrics.rowHeight;
	const int lastRow = (scrollY + viewport()->height()) / _metrics.rowHeight + 1;
	const int rows =
		(_data.size() + _metrics.bytesPerRow - 1) / _metrics.bytesPerRow;
	const int maxRow = qMin(lastRow, rows);

	const QColor textCol = palette().color(QPalette::Text);
	const QColor hlCol = palette().color(QPalette::Highlight);
	const QColor hlTextCol = palette().color(QPalette::HighlightedText);
	const QColor dupCol(0x55, 0x55, 0x88); // тёмно-синий для дубликатов

	for (int row = firstRow; row < maxRow; ++row) {
		const int y =
			row * _metrics.rowHeight - scrollY + _metrics.rowHeight / 2 + 2;

		// Offset
		QString offsetStr = QString("%1")
			.arg(row * _metrics.bytesPerRow, 8, 16, QChar('0'))
			.toUpper();
		p.setPen(textCol);
		p.drawText(0, y, offsetStr);

		// Hex-байты
		int x = _metrics.hexStartX;
		for (int col = 0; col < _metrics.bytesPerRow; ++col) {
			const qint64 idx = static_cast<qint64>(row) * _metrics.bytesPerRow + col;
			const bool inRange = idx < _data.size();
			bool isSelected = false;

			if (_selStart >= 0) {
				const qint64 start = qMin(_selStart, _selEnd);
				const qint64 end = qMax(_selStart, _selEnd);
				isSelected = (idx >= start && idx <= end);
			}

			if (isSelected) {
				p.fillRect(x - 2, y - _metrics.rowHeight / 2 - 2,
					_metrics.charWidth * 2 + 2, _metrics.rowHeight - 2, hlCol);
				p.setPen(hlTextCol);
			}
			else {
				p.setPen(textCol);
			}

			if (inRange) {
				const quint8 byte = static_cast<quint8>(_data[idx]);
				QString hex = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

				// Подсветка дубликатов
				if (_highlightDuplicates && _byteFrequency.value(byte, 0) > 1) {
					if (!isSelected) {
						p.setPen(dupCol);
					}
				}

				p.drawText(x, y, hex);
			}
			else {
				p.drawText(x, y, "  ");
			}

			x += _metrics.charWidth * 2 + 4;
		}

		// ASCII
		x = _metrics.asciiStartX;
		for (int col = 0; col < _metrics.bytesPerRow; ++col) {
			const qint64 idx = static_cast<qint64>(row) * _metrics.bytesPerRow + col;
			if (idx < _data.size()) {

				bool isSelected = false;
				if (_selStart >= 0) {
					const qint64 start = qMin(_selStart, _selEnd);
					const qint64 end = qMax(_selStart, _selEnd);
					isSelected = (idx >= start && idx <= end);
				}

				char c = _data[idx];
				QChar ch = (c >= 32 && c <= 126) ? QChar(c) : '.';
				if (isSelected) {
					p.fillRect(
						x - 1,
						y - _metrics.rowHeight / 2 - 2,
						_metrics.charWidth + 2,
						_metrics.rowHeight - 2,
						hlCol
					);
					p.setPen(hlTextCol);
				}
				else {
					p.setPen(textCol);
				}

				p.drawText(x, y, ch);
			}

			x += _metrics.charWidth + 2;
		}
	}
}

void HexDumpView::resizeEvent(QResizeEvent*) {
	const int rows =
		(_data.size() + _metrics.bytesPerRow - 1) / _metrics.bytesPerRow;
	const int contentHeight = rows * _metrics.rowHeight;
	verticalScrollBar()->setRange(0,
		qMax(0, contentHeight - viewport()->height()));
}

void HexDumpView::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		_selStart = _selEnd = posToByte(event->pos());
		_dragging = true;
		emitSelectionChange();
		viewport()->update();
	}
}

void HexDumpView::mouseMoveEvent(QMouseEvent* event) {
	if (_dragging && (event->buttons() & Qt::LeftButton)) {
		_selEnd = posToByte(event->pos());
		emitSelectionChange();
		viewport()->update();
	}
}

void HexDumpView::mouseReleaseEvent(QMouseEvent*) {
	_dragging = false;
}

void HexDumpView::wheelEvent(QWheelEvent* event) {
	// Плавная прокрутка (Qt6)
	QPoint angleDelta = event->angleDelta();
	int delta = angleDelta.y() ? angleDelta.y() : angleDelta.x();
	int step = delta / 4; // стандартное сглаживание
	verticalScrollBar()->setValue(verticalScrollBar()->value() - step);
	event->accept();
}

void HexDumpView::keyPressEvent(QKeyEvent* event) {
	if (event->modifiers() == Qt::ControlModifier) {
		if (event->key() == Qt::Key_C) {
			if (_selStart >= 0) {
				QByteArray sel = selectedData();
				if (!sel.isEmpty()) {
					QClipboard* clipboard = QApplication::clipboard();

					// Форматы:
					QString hexStr;
					QString asciiStr;
					QString cArray;

					for (int i = 0; i < sel.size(); ++i) {
						quint8 b = static_cast<quint8>(sel[i]);
						hexStr += QString("%1 ").arg(b, 2, 16, QChar('0')).toUpper();
						asciiStr += (b >= 32 && b <= 126) ? QChar(b) : '.';
						cArray += QString("0x%1, ").arg(b, 2, 16, QChar('0')).toUpper();
					}

					// Убираем завершающий пробел/запятую
					if (!hexStr.isEmpty())
						hexStr.chop(1);
					if (!cArray.isEmpty())
						cArray.chop(2);

					QString full = QString("hex: %1\n"
						"ascii: %2\n"
						"c: {%3}")
						.arg(hexStr, asciiStr, cArray);

					clipboard->setText(full);
				}
			}
			event->accept();
			return;
		}
	}
	QWidget::keyPressEvent(event);
}
