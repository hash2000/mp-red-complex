#include "Game/widgets/map_view/create_map_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>
#include <QClipboard>
#include <QApplication>
#include <QUuid>

class CreateMapDialog::Private {
public:
    Private(CreateMapDialog* parent) : q(parent) {}
    CreateMapDialog* q;

    // UI элементы
    QLineEdit* idEdit = nullptr;
    QPushButton* copyIdButton = nullptr;
    QLineEdit* nameEdit = nullptr;
    QSpinBox* chunkWidthSpin = nullptr;
    QSpinBox* chunkHeightSpin = nullptr;
    QSpinBox* tileWidthSpin = nullptr;
    QSpinBox* tileHeightSpin = nullptr;

    QString mapId;
};

CreateMapDialog::CreateMapDialog(QWidget* parent)
    : QDialog(parent)
    , d(std::make_unique<Private>(this)) {
    setWindowTitle("Создание новой карты");
    setModal(true);
    resize(400, 300);

    // Генерируем ID
    d->mapId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    setupUI();
}

CreateMapDialog::~CreateMapDialog() = default;

void CreateMapDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Группа: Основная информация
    auto* mainGroup = new QGroupBox("Основная информация");
    auto* mainForm = new QFormLayout(mainGroup);

    // ID карты (readonly, серый фон)
    d->idEdit = new QLineEdit(d->mapId);
    d->idEdit->setReadOnly(true);
    d->idEdit->setToolTip("Уникальный идентификатор карты");

    // Кнопка копирования с Unicode иконкой
    d->copyIdButton = new QPushButton("\uE8EC"); // Unicode clipboard icon
    d->copyIdButton->setToolTip("Копировать идентификатор");
    d->copyIdButton->setFixedSize(32, 24);

		connect(d->copyIdButton, &QPushButton::clicked, this, &CreateMapDialog::onCopyIdClicked);

    // Layout для ID и кнопки
    auto* idLayout = new QHBoxLayout();
    idLayout->addWidget(d->idEdit);
    idLayout->addWidget(d->copyIdButton);
    mainForm->addRow("ID карты:", idLayout);

    // Имя карты
    d->nameEdit = new QLineEdit();
    d->nameEdit->setPlaceholderText("Введите имя карты");
    d->nameEdit->setToolTip("Имя карты (будет использоваться как идентификатор в файловой системе)");
    mainForm->addRow("Имя карты:", d->nameEdit);

    mainLayout->addWidget(mainGroup);

    // Группа: Размер чанка
    auto* chunkGroup = new QGroupBox("Размер чанка (в тайлах)");
    auto* chunkLayout = new QHBoxLayout(chunkGroup);

    d->chunkWidthSpin = new QSpinBox();
    d->chunkWidthSpin->setRange(1, 128);
    d->chunkWidthSpin->setValue(32);
    d->chunkWidthSpin->setToolTip("Ширина чанка в тайлах");
    chunkLayout->addWidget(new QLabel("Ширина:"));
    chunkLayout->addWidget(d->chunkWidthSpin);

    d->chunkHeightSpin = new QSpinBox();
    d->chunkHeightSpin->setRange(1, 128);
    d->chunkHeightSpin->setValue(32);
    d->chunkHeightSpin->setToolTip("Высота чанка в тайлах");
    chunkLayout->addWidget(new QLabel("Высота:"));
    chunkLayout->addWidget(d->chunkHeightSpin);

    mainLayout->addWidget(chunkGroup);

    // Группа: Размер тайла
    auto* tileGroup = new QGroupBox("Размер одного тайла (в пикселях)");
    auto* tileLayout = new QHBoxLayout(tileGroup);

    d->tileWidthSpin = new QSpinBox();
    d->tileWidthSpin->setRange(1, 256);
    d->tileWidthSpin->setValue(32);
    d->tileWidthSpin->setToolTip("Ширина тайла в пикселях");
    tileLayout->addWidget(new QLabel("Ширина:"));
    tileLayout->addWidget(d->tileWidthSpin);

    d->tileHeightSpin = new QSpinBox();
    d->tileHeightSpin->setRange(1, 256);
    d->tileHeightSpin->setValue(32);
    d->tileHeightSpin->setToolTip("Высота тайла в пикселях");
    tileLayout->addWidget(new QLabel("Высота:"));
    tileLayout->addWidget(d->tileHeightSpin);

    mainLayout->addWidget(tileGroup);

    // Кнопки
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    auto* cancelButton = new QPushButton("Отмена");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);

    auto* okButton = new QPushButton("Создать");
    okButton->setDefault(true);
    okButton->setStyleSheet(
        "QPushButton { "
        "  background-color: #3b82f6; "
        "  color: white; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 6px 16px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #2563eb; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #1d4ed8; "
        "}"
    );
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(okButton);

    mainLayout->addLayout(buttonLayout);
}

void CreateMapDialog::onCopyIdClicked() {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(d->mapId);
}

QString CreateMapDialog::getMapName() const {
    return d->nameEdit->text().trimmed();
}

int CreateMapDialog::getChunkWidth() const {
    return d->chunkWidthSpin->value();
}

int CreateMapDialog::getChunkHeight() const {
    return d->chunkHeightSpin->value();
}

int CreateMapDialog::getTileWidth() const {
    return d->tileWidthSpin->value();
}

int CreateMapDialog::getTileHeight() const {
    return d->tileHeightSpin->value();
}
