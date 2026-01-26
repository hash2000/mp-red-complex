#include "Launcher/widgets/actions_widget.h"
#include <QVBoxLayout>

ActionsWidget::ActionsWidget(QWidget* parent)
: QWidget(parent) {
	auto layout = new QVBoxLayout(this);

	biomeLabel = new QLabel("Направление не выбрано");
	resourcesLabel = new QLabel("");
	digButton = new QPushButton("Добыча");

	layout->addWidget(biomeLabel);
	layout->addWidget(resourcesLabel);
	layout->addWidget(digButton);
	layout->addStretch();

	connect(digButton, &QPushButton::clicked, this, &ActionsWidget::digRequested);
}

void ActionsWidget::onTileInDirectionChanged(const Tile& tile) {
	currentTile = tile;

	if (tile._biome == BiomeType::Empty && tile._destruction == 0.0f) {
		biomeLabel->setText("Направление не выбрано");
		resourcesLabel->setText("");
		digButton->setEnabled(false);
		return;
	}

	QString biomeName = Tile::biomeName(tile._biome);
	biomeLabel->setText(QString("Биом: %1").arg(biomeName));

	QString resText;
	if (tile._resources.empty()) {
		resText = "Ресурсы: отсутствуют";
	}
	else {
		QStringList parts;
		for (const auto& r : tile._resources) {
			parts << QString("%1: %2%")
				.arg(r._name)
				.arg(r._percentage, 0, 'f', 1);
		}

		resText = "Ресурсы:\n" + parts.join("\n");
	}
	resourcesLabel->setText(resText);

	digButton->setEnabled(true);
}
