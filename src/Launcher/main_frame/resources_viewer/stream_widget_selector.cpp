#include "Launcher/main_frame/resources_viewer/stream_widget_selector.h"
#include "Game/data_format/int/data_reader.h"
#include <QLabel>
#include <QSpinBox>
#include <QWidget>
#include <QLineEdit>

StreamWidgetSelector::StreamWidgetSelector(std::weak_ptr<Resources> resources,
		QStackedWidget *centerStack,
		QVBoxLayout *actionsLayout)
: _resources(resources)
, _centerStack(centerStack)
, _actionsLayout(actionsLayout) {
}

void StreamWidgetSelector::buildStackedView() {
  _views.empty = new QWidget;
	_views.hex = new HexDumpWidget;

	_centerStack->addWidget(_views.empty);
	_centerStack->addWidget(_views.hex);

	_centerStack->setCurrentWidget(_views.empty);


  connect(_views.hex, &HexDumpWidget::selectionChanged,
          this, &StreamWidgetSelector::onHexSelectionChanged);
}

void StreamWidgetSelector::setSelection(const QStandardItem *item) {
  if (!item) {
		_selection.type = AssetsViewItemType::Undefined;
		return;
	}

	const auto index = item->index();
	_selection.container = index.data(static_cast<int>(AssetsViewItemRole::ContainerName)).toString();
	_selection.path = index.data(static_cast<int>(AssetsViewItemRole::FullPath)).toString();
	_selection.suffix = index.data(static_cast<int>(AssetsViewItemRole::Suffix)).toString();
	_selection.type = index.data(static_cast<int>(AssetsViewItemRole::Type)).value<AssetsViewItemType>();
}

bool StreamWidgetSelector::isSelected() const {
  return _selection.type != AssetsViewItemType::Undefined;
}

std::unique_ptr<QMenu> StreamWidgetSelector::buildContextMenu(QWidget *parent) const {
	auto menu = std::make_unique<QMenu>(parent);
	if (_selection.type == AssetsViewItemType::File) {
		menu
			->addAction("Hex View")
			->setObjectName("action_hex_view");
	}

	return menu;
}

void StreamWidgetSelector::displayModel() {
	const auto stream = getStream();
	if (!stream) {
		applyEmptyView();
		return;
	}

	auto widget = buildWidget(_selection.suffix, stream.value());
	if (!widget) {
		return;
	}

	_centerStack->setCurrentWidget(widget);
}

void StreamWidgetSelector::displayHexView() {
	const auto streamOpt = getStream();
	if (!streamOpt) {
		applyEmptyView();
		return;
	}

	DataStream& stream = streamOpt.value();
	auto block = stream.readBlockAsQByteArray();

	_views.hex->setData(block);
	_centerStack->setCurrentWidget(_views.hex);
	applyHexControls();
}

QWidget *StreamWidgetSelector::buildWidget(const QString &suffix, DataStream &stream) {
	cleanViewsData();
	auto block = stream.makeBlockAsStream();
	if (suffix == "int") {
		Format::Int::Programmability result;
		Format::Int::DataReader reader(*block);
		reader.read(result);
	}

  return _views.empty;
}

std::optional<std::reference_wrapper<DataStream>> StreamWidgetSelector::getStream()
{
	const auto resources = _resources.lock();
	if (_selection.type != AssetsViewItemType::File) {
		return std::nullopt;
	}

	if (!resources) {
		return std::nullopt;
	}

	return resources->getStream(_selection.container, _selection.path);
}

void StreamWidgetSelector::cleanViewsData(QWidget *current) {
  QLayoutItem *item;
  while ((item = _actionsLayout->takeAt(0)) != nullptr) {
    if (item->widget()) {
      item->widget()->setParent(nullptr);
      delete item->widget();
    }
    delete item;
  }

	if (_views.hex != current) {
		_views.hex->clear();
	}
}

void StreamWidgetSelector::applyEmptyView() {
	cleanViewsData();
	_centerStack->setCurrentWidget(_views.empty);
}

void StreamWidgetSelector::onHexSelectionChanged(qint64 offset, qint64 length, const QByteArray& selected) {
	if (selected.isEmpty()) {
		return;
	}
}

void StreamWidgetSelector::applyHexControls() {
	cleanViewsData(_views.hex);
	auto hexWidget = _views.hex;

	QWidget *controlWidget = new QWidget;
  QGridLayout *grid = new QGridLayout(controlWidget);
  grid->setContentsMargins(0, 0, 0, 0);
  grid->setSpacing(4);

  // --- Offset ---
  QLabel *offsetLabel = new QLabel("Offset:", controlWidget);
  QSpinBox *offsetSpinBox = new QSpinBox(controlWidget);
  offsetSpinBox->setRange(0, hexWidget->getDataSize());
  offsetSpinBox->setSuffix(" bytes");
  offsetSpinBox->setAccelerated(true);

  // --- Length ---
  QLabel *lengthLabel = new QLabel("Length:", controlWidget);
  QSpinBox *lengthSpinBox = new QSpinBox(controlWidget);
  lengthSpinBox->setRange(0, hexWidget->getDataSize());
  lengthSpinBox->setSuffix(" bytes");
  lengthSpinBox->setAccelerated(true);

  // --- Read-only value display ---
  QLabel *valueLabel = new QLabel("Value:", controlWidget);
  QLineEdit *valueEdit = new QLineEdit(controlWidget);
  valueEdit->setReadOnly(true);
  valueEdit->setPlaceholderText("Select bytes to view value");

  // Расположение в сетке
  grid->addWidget(offsetLabel,    0, 0, Qt::AlignRight);
  grid->addWidget(offsetSpinBox,  0, 1);
  grid->addWidget(lengthLabel,    1, 0, Qt::AlignRight);
  grid->addWidget(lengthSpinBox,  1, 1);
  grid->addWidget(valueLabel,     2, 0, Qt::AlignRight);
  grid->addWidget(valueEdit,      2, 1);

  // Обновление диапазонов при изменении размера буфера
  auto updateRanges = [hexWidget, offsetSpinBox, lengthSpinBox]() {
    int bufSize = hexWidget->getDataSize();
    offsetSpinBox->setMaximum(bufSize);
    lengthSpinBox->setMaximum(bufSize);
  };

  updateRanges();

  // Обработка изменений offset/length
  auto updateSelection = [hexWidget, offsetSpinBox, lengthSpinBox, valueEdit, this]() {
    const qsizetype offset = offsetSpinBox->value();
    qsizetype length = lengthSpinBox->value();

    // Ограничение: offset + length не должен превышать размер буфера
    const auto maxLen = hexWidget->getDataSize() - offset;
    if (length > maxLen) {
      length = maxLen;
      lengthSpinBox->setValue(length);
    }

    // Прокрутка к байту и выделение
    hexWidget->selectRange(offset, length);
    hexWidget->scrollToByte(offset);

    // Обновление отображения значения
    if (length > 0) {
			const auto stream = getStream();
			if (!stream) {
				return;
			}

			auto &pStream = stream->get();
			auto block = pStream.makeBlockAsStream();
			block->position(offset);

			switch (length) {
			case sizeof(int8_t): {
				const auto value = block->i8();
				valueEdit->setText(QString("%1").arg(value));
				}
				break;
			case sizeof(int16_t): {
				const auto value = block->i16();
				valueEdit->setText(QString("%1").arg(value));
				}
				break;
			case sizeof(int32_t): {
				const auto value = block->i32();
				valueEdit->setText(QString("%1").arg(value));
				}
				break;
			default: {
				const auto selected = hexWidget->selectedData();
				const auto value = selected.toHex(' ').toUpper();
				valueEdit->setText(QString("%1 (%2 bytes)").arg(value).arg(selected.size()));
			}
			}
    } else {
      valueEdit->clear();
      valueEdit->setPlaceholderText("Select bytes to view value");
    }
  };

  connect(offsetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, updateSelection);
  connect(lengthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
          this, updateSelection);

  _actionsLayout->addWidget(controlWidget);
  _actionsLayout->addStretch();

  updateSelection();
}
