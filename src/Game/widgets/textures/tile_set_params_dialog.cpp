#include "Game/widgets/textures/tile_set_params_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>

class TileSetParamsDialog::Private {
public:
	Private(TileSetParamsDialog* parent) : q(parent) {}
	TileSetParamsDialog* q;

	// Элементы управления
	QLineEdit* gridSizeXEdit = nullptr;
	QLineEdit* gridSizeYEdit = nullptr;
	QCheckBox* showGridCheckBox = nullptr;
	QPushButton* applyButton = nullptr;

	// Состояние
	int tilesCountX = 0;
	int tilesCountY = 0;
	bool showGrid = true;
};

TileSetParamsDialog::TileSetParamsDialog(int tilesCountX, int tilesCountY, QWidget* parent)
	: QDialog(parent)
	, d(std::make_unique<Private>(this)) {
	d->tilesCountX = tilesCountX;
	d->tilesCountY = tilesCountY;

	setWindowTitle("Параметры тайлового набора");
	setModal(true);
	setMinimumWidth(300);
	setStyleSheet(R"(
		QDialog {
			background-color: #1a202c;
		}
	)");

	setupLayout();
}

TileSetParamsDialog::~TileSetParamsDialog() = default;

void TileSetParamsDialog::setupLayout() {
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(12, 12, 12, 12);
	mainLayout->setSpacing(12);

	// === Форма параметров ===
	auto* formLayout = new QFormLayout();
	formLayout->setSpacing(8);

	// Размер X
	auto* xLayout = new QHBoxLayout();
	auto* xLabel = new QLabel("Столбцы (X):");
	xLabel->setStyleSheet("color: #a0aec0; font-size: 12px;");
	xLayout->addWidget(xLabel);

	d->gridSizeXEdit = new QLineEdit(QString::number(d->tilesCountX));
	d->gridSizeXEdit->setMaximumWidth(80);
	d->gridSizeXEdit->setStyleSheet(R"(
		QLineEdit {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 4px 8px;
			font-size: 12px;
		}
		QLineEdit:focus {
			border: 1px solid #63b3ed;
		}
	)");
	xLayout->addWidget(d->gridSizeXEdit);
	xLayout->addStretch();
	formLayout->addRow(xLayout);

	// Размер Y
	auto* yLayout = new QHBoxLayout();
	auto* yLabel = new QLabel("Строки (Y):");
	yLabel->setStyleSheet("color: #a0aec0; font-size: 12px;");
	yLayout->addWidget(yLabel);

	d->gridSizeYEdit = new QLineEdit(QString::number(d->tilesCountY));
	d->gridSizeYEdit->setMaximumWidth(80);
	d->gridSizeYEdit->setStyleSheet(R"(
		QLineEdit {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 4px 8px;
			font-size: 12px;
		}
		QLineEdit:focus {
			border: 1px solid #63b3ed;
		}
	)");
	yLayout->addWidget(d->gridSizeYEdit);
	yLayout->addStretch();
	formLayout->addRow(yLayout);

	mainLayout->addLayout(formLayout);

	// Чекбокс показа сетки
	d->showGridCheckBox = new QCheckBox("Отображать сетку");
	d->showGridCheckBox->setChecked(d->showGrid);
	d->showGridCheckBox->setStyleSheet(R"(
		QCheckBox {
			color: #e2e8f0;
			font-size: 12px;
			spacing: 8px;
		}
		QCheckBox::indicator {
			width: 16px;
			height: 16px;
			border: 1px solid #4a5568;
			border-radius: 3px;
			background-color: #2d3748;
		}
		QCheckBox::indicator:checked {
			background-color: #4299e1;
			border: 1px solid #63b3ed;
		}
	)");
	mainLayout->addWidget(d->showGridCheckBox);

	// Кнопка Применить
	d->applyButton = new QPushButton("⚙ Применить");
	d->applyButton->setStyleSheet(R"(
		QPushButton {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 6px 12px;
			font-size: 12px;
		}
		QPushButton:hover {
			background-color: #4a5568;
			border: 1px solid #718096;
		}
		QPushButton:pressed {
			background-color: #1a202c;
		}
	)");
	mainLayout->addWidget(d->applyButton);

	// Сигналы
	connect(d->applyButton, &QPushButton::clicked,
	        this, &TileSetParamsDialog::onApplyClicked);
}

int TileSetParamsDialog::gridSizeX() const {
	const auto text = d->gridSizeXEdit->text();
	const int value = text.toInt();
	return value > 0 ? value : d->tilesCountX;
}

int TileSetParamsDialog::gridSizeY() const {
	const auto text = d->gridSizeYEdit->text();
	const int value = text.toInt();
	return value > 0 ? value : d->tilesCountY;
}

bool TileSetParamsDialog::showGrid() const {
	return d->showGridCheckBox->isChecked();
}

void TileSetParamsDialog::onApplyClicked() {
	const int newX = gridSizeX();
	const int newY = gridSizeY();

	if (newX > 0 && newY > 0 && newX <= 128 && newY <= 128) {
		d->tilesCountX = newX;
		d->tilesCountY = newY;
		d->showGrid = showGrid();
		emit settingsApplied(d->tilesCountX, d->tilesCountY, d->showGrid);
		accept();
	}
}
