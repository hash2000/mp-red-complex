#include "Launcher/widgets/actions_widget.h"
#include <QVBoxLayout>

ActionsWidget::ActionsWidget(QWidget* parent)
: QWidget(parent) {
	auto layout = new QVBoxLayout(this);

	_biomeLabel = new QLabel("Направление не выбрано");
	_resourcesLabel = new QLabel("");
	_digButton = new QPushButton("Добыча");
	_searchPlayerButton = new QPushButton("Центрировать");

	layout->addWidget(_biomeLabel);
	layout->addWidget(_resourcesLabel);
	layout->addWidget(_digButton);
	layout->addWidget(_searchPlayerButton);
	layout->addStretch();

	connect(_digButton, &QPushButton::clicked, this, &ActionsWidget::digRequested);
	connect(_searchPlayerButton, &QPushButton::clicked, this, &ActionsWidget::centerOnPlayerRequested);
}

void ActionsWidget::onTileInDirectionChanged(const Tile& tile) {
	_currentTile = tile;

	if (tile._biome == BiomeType::Empty && tile._destruction == 0.0f) {
		_biomeLabel->setText("Направление не выбрано");
		_resourcesLabel->setText("");
		_digButton->setEnabled(false);
		return;
	}

	QString biomeName = Tile::biomeName(tile._biome);
	_biomeLabel->setText(QString("Биом: %1").arg(biomeName));

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

	_resourcesLabel->setText(resText);
	_digButton->setEnabled(true);
}
