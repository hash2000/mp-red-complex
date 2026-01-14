#include "ResourcesTool/widgets/hex/hex_control_panel.h"
#include "ResourcesTool/widgets/hex/hex_dump_view.h"
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <qpushbutton.h>

HexControlPanel::HexControlPanel(HexDumpView *hexWidget, std::shared_ptr<DataStream> stream, QWidget *parent)
: QWidget(parent)
, _hexWidget(hexWidget)
, _stream(stream) {
  setObjectName("HexControlPanel");

  auto *grid = new QGridLayout(this);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setSpacing(4);

  QLabel *offsetLabel = new QLabel("Offset:", this);
  _offsetSpinBox = new QSpinBox(this);
  _offsetSpinBox->setRange(0, _hexWidget->getDataSize());
  _offsetSpinBox->setSuffix(" bytes");
  _offsetSpinBox->setAccelerated(true);

  QLabel *lengthLabel = new QLabel("Length:", this);
  _lengthSpinBox = new QSpinBox(this);
  _lengthSpinBox->setRange(0, _hexWidget->getDataSize());
  _lengthSpinBox->setSuffix(" bytes");
  _lengthSpinBox->setAccelerated(true);

  QLabel *valueLabel = new QLabel("Signed:", this);
  _valueEdit = new QLineEdit(this);
  _valueEdit->setReadOnly(true);
  _valueEdit->setPlaceholderText("Select bytes to view value");

  QLabel *valueUnsignedLabel = new QLabel("Unsigned:", this);
  _valueUnsignedEdit = new QLineEdit(this);
  _valueUnsignedEdit->setReadOnly(true);
  _valueUnsignedEdit->setPlaceholderText("Select bytes to view value");

	_nextButton = new QPushButton(">>", this);
	_prevButton = new QPushButton("<<", this);

  grid->addWidget(offsetLabel, 0, 0, Qt::AlignRight);
  grid->addWidget(_offsetSpinBox, 0, 1);
  grid->addWidget(lengthLabel, 1, 0, Qt::AlignRight);
  grid->addWidget(_lengthSpinBox, 1, 1);
  grid->addWidget(valueLabel, 2, 0, Qt::AlignRight);
  grid->addWidget(_valueEdit, 2, 1);
  grid->addWidget(valueUnsignedLabel, 3, 0, Qt::AlignRight);
  grid->addWidget(_valueUnsignedEdit, 3, 1);
	grid->addWidget(_prevButton, 4, 0);
	grid->addWidget(_nextButton, 4, 1);
  grid->setRowStretch(5, 1);

  connect(_offsetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &HexControlPanel::onUpdateSelection);
  connect(_lengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, &HexControlPanel::onUpdateSelection);

	connect(_nextButton, &QPushButton::pressed,
					this, &HexControlPanel::onNextBytesGroup);

	connect(_prevButton, &QPushButton::pressed,
					this, &HexControlPanel::onPrevBytesGroup);

	connect(_hexWidget, &HexDumpView::selectionChanged,
					this, &HexControlPanel::onHexSelectionChanged);

  onUpdateSelection();
}

void HexControlPanel::updateFromSelection() {
	const auto [start, end] = _hexWidget->selectedRange();
  _offsetSpinBox->setValue(static_cast<int>(start));
  _lengthSpinBox->setValue(static_cast<int>(end));
  onUpdateSelection();
}

void HexControlPanel::onUpdateSelection() {
  const qsizetype offset = _offsetSpinBox->value();
  qsizetype length = _lengthSpinBox->value();

  const auto bufSize = _hexWidget->getDataSize();
  const auto maxLen = bufSize - offset;

  if (length > maxLen) {
    length = maxLen;
    _lengthSpinBox->setValue(static_cast<int>(length));
  }

  _hexWidget->selectRange(offset, length);
  _hexWidget->scrollToByte(offset);

  emit selectionChanged(offset, length);

  if (length == 0) {
    _valueEdit->clear();
    _valueEdit->setPlaceholderText("Select bytes to view value");
    _valueUnsignedEdit->clear();
    _valueUnsignedEdit->setPlaceholderText("Select bytes to view value");
    return;
  }

  if (!_stream) {
    _valueEdit->setText("<no stream>");
    _valueUnsignedEdit->setText("<no stream>");
    return;
  }

  _stream->position(offset);

  QString signedStr, unsignedStr;

  switch (length) {
    case sizeof(int8_t): {
      const auto pos = _stream->position();
      const auto sv = _stream->i8();
      _stream->position(pos);
      const auto uv = _stream->u8();
      signedStr = QString::number(sv);
      unsignedStr = QString::number(uv);
      break;
    }
    case sizeof(int16_t): {
      const auto pos = _stream->position();
      const auto sv = _stream->i16();
      _stream->position(pos);
      const auto uv = _stream->u16();
      signedStr = QString::number(sv);
      unsignedStr = QString::number(uv);
      break;
    }
    case sizeof(int32_t): {
      const auto pos = _stream->position();
      const auto sv = _stream->i32();
      _stream->position(pos);
      const auto uv = _stream->u32();
      signedStr = QString::number(sv);
      unsignedStr = QString::number(uv);
      break;
    }
    default: {
      const auto selected = _hexWidget->selectedData();
      const auto hex = selected.toHex(' ').toUpper();
      const QString common = QString("%1 (%2 bytes)").arg(QString::fromLatin1(hex)).arg(selected.size());
      signedStr = common;
      unsignedStr = common;
      break;
    }
  }

  _valueEdit->setText(signedStr);
  _valueUnsignedEdit->setText(unsignedStr);
}

void HexControlPanel::onNextBytesGroup() {
	const auto [start, end] = _hexWidget->selectedRange();
	_offsetSpinBox->setValue(static_cast<int>(start + end));
}

void HexControlPanel::onPrevBytesGroup() {
	const auto [start, end] = _hexWidget->selectedRange();
	_offsetSpinBox->setValue(static_cast<int>(start - end));
}

void HexControlPanel::onHexSelectionChanged(qint64 offset, qint64 length, const QByteArray& selected) {
	const auto [start, end] = _hexWidget->selectedRange();
  _offsetSpinBox->setValue(static_cast<int>(start));
  _lengthSpinBox->setValue(static_cast<int>(end));
}
