#include "Launcher/widgets/legend_widget.h"
#include "Launcher/map/tile.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>

LegendWidget::LegendWidget(QWidget* parent) {
	auto layout = new QVBoxLayout(this);
	layout->setSpacing(4);
	layout->setContentsMargins(6, 6, 6, 6);

	// Перебираем все биомы
	for (int i = static_cast<int>(BiomeType::Empty);
		i <= static_cast<int>(BiomeType::TitaniumDeposit); ++i) {
		BiomeType b = static_cast<BiomeType>(i);

		// Создаём временный тайл, чтобы получить цвет
		Tile temp;
		temp._biome = b;
		QColor color = temp.color();
		QString name = Tile::biomeName(b);

		// Цветной квадратик
		QPixmap pixmap(16, 16);
		pixmap.fill(color);

		// Метка с пиксмапом и текстом
		auto colorLabel = new QLabel();
		colorLabel->setPixmap(pixmap);
		colorLabel->setFixedSize(16, 16);

		auto textLabel = new QLabel(name);

		auto rowLayout = new QHBoxLayout();
		rowLayout->addWidget(colorLabel);
		rowLayout->addWidget(textLabel);
		rowLayout->addStretch();

		auto rowWidget = new QWidget();
		rowWidget->setLayout(rowLayout);

		layout->addWidget(rowWidget);
	}

	layout->addStretch();
	setLayout(layout);
}
