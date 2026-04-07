#include "Game/widgets/textures/texture_editor_widget.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>

class TextureEditorWidget::Private {
public:
	Private(TextureEditorWidget* parent) : q(parent) {}
	TextureEditorWidget* q;

	TexturesService* texturesService = nullptr;

	// Компоненты UI
	QSplitter* splitter = nullptr;
	QLabel* previewLabel = nullptr;
	QComboBox* textureTypeCombo = nullptr;
	QListWidget* textureList = nullptr;

	// Пагинация
	static constexpr int kPageSize = 50;
	int currentPage = 0;
	QStringList allTextures;
	TextureType currentType = TextureType::Icon;
};

TextureEditorWidget::TextureEditorWidget(TexturesService* texturesService, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->texturesService = texturesService;

	setObjectName("TextureEditorWidget");
	setupLayout();

	// Загружаем первый тип текстур
	d->currentType = TextureType::Icon;
	d->textureTypeCombo->setCurrentIndex(0);
	updateTextureList();
}

TextureEditorWidget::~TextureEditorWidget() = default;

void TextureEditorWidget::setupLayout() {
	auto* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(8);

	// Создаём сплиттер
	d->splitter = new QSplitter(Qt::Horizontal, this);

	// Левая часть - превью
	auto* previewWidget = new QWidget(d->splitter);
	auto* previewLayout = new QVBoxLayout(previewWidget);
	previewLayout->setContentsMargins(0, 0, 0, 0);

	d->previewLabel = new QLabel("Выберите текстуру", previewWidget);
	d->previewLabel->setAlignment(Qt::AlignCenter);
	d->previewLabel->setObjectName("TexturePreviewLabel");
	d->previewLabel->setStyleSheet(R"(
		#TexturePreviewLabel {
			background-color: #1a202c;
			border: 1px solid #4a5568;
			border-radius: 8px;
		}
	)");
	previewLayout->addWidget(d->previewLabel);

	// Правая часть - список
	auto* listWidget = new QWidget(d->splitter);
	auto* listLayout = new QVBoxLayout(listWidget);
	listLayout->setContentsMargins(0, 0, 0, 0);
	listLayout->setSpacing(4);

	// Выбор типа текстур
	auto* typeLabel = new QLabel("Тип текстур:", listWidget);
	typeLabel->setStyleSheet("color: #a0aec0; font-size: 11px; padding: 2px 0;");
	listLayout->addWidget(typeLabel);

	d->textureTypeCombo = new QComboBox(listWidget);
	d->textureTypeCombo->setObjectName("TextureTypeCombo");
	d->textureTypeCombo->addItem("Иконки");
	d->textureTypeCombo->addItem("Сущности");
	d->textureTypeCombo->addItem("Предметы");
	d->textureTypeCombo->addItem("Персонажи");
	d->textureTypeCombo->addItem("Экипировка");
	d->textureTypeCombo->addItem("Пользователи");
	d->textureTypeCombo->addItem("Тайлы");

	d->textureTypeCombo->setStyleSheet(R"(
		#TextureTypeCombo {
			background-color: #2d3748;
			color: #e2e8f0;
			border: 1px solid #4a5568;
			border-radius: 4px;
			padding: 4px;
		}
		#TextureTypeCombo::drop-down {
			border: none;
		}
		#TextureTypeCombo QAbstractItemView {
			background-color: #2d3748;
			color: #e2e8f0;
			selection-background-color: #4a5568;
		}
	)");
	listLayout->addWidget(d->textureTypeCombo);

	// Список текстур с ленивой загрузкой
	d->textureList = new QListWidget(listWidget);
	d->textureList->setObjectName("TextureList");
	d->textureList->setStyleSheet(R"(
		#TextureList {
			background-color: #1a202c;
			border: 1px solid #4a5568;
			border-radius: 4px;
		}
		#TextureList::item {
			padding: 6px;
			border-bottom: 1px solid #2d3748;
		}
		#TextureList::item:selected {
			background-color: #4a5568;
		}
		#TextureList::item:hover {
			background-color: #2d3748;
		}
		#TextureList QScrollBar:vertical {
			background-color: #2d3748;
			width: 8px;
			border-radius: 4px;
		}
		#TextureList QScrollBar::handle:vertical {
			background-color: #4a5568;
			border-radius: 4px;
			min-height: 20px;
		}
		#TextureList QScrollBar::handle:vertical:hover {
			background-color: #718096;
		}
		#TextureList QScrollBar::add-line:vertical, #TextureList QScrollBar::sub-line:vertical {
			height: 0px;
		}
	)");
	listLayout->addWidget(d->textureList);

	// Подключаем сплиттер
	mainLayout->addWidget(d->splitter);

	// Устанавливаем пропорции сплиттера (70% превью, 30% список)
	d->splitter->setStretchFactor(0, 3);
	d->splitter->setStretchFactor(1, 1);

	// Сигналы
	connect(d->textureTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
	        this, &TextureEditorWidget::onTextureTypeChanged);
	connect(d->textureList, &QListWidget::itemClicked,
	        this, [this](QListWidgetItem* item) {
		onTextureItemSelected(item);
	});
	connect(d->textureList->verticalScrollBar(), &QScrollBar::valueChanged,
	        this, [this](int value) {
		// Ленивая загрузка при прокрутке
		if (value >= d->textureList->verticalScrollBar()->maximum() - 5) {
			loadTexturesPage();
		}
	});
}

void TextureEditorWidget::onTextureTypeChanged(int index) {
	d->currentType = static_cast<TextureType>(index);
	d->currentPage = 0;
	d->allTextures.clear();
	d->textureList->clear();
	updateTextureList();
}

void TextureEditorWidget::onTextureItemSelected(QListWidgetItem* current) {
	if (current) {
		const QString fileName = current->text();
		updatePreview(fileName);
		emit textureSelected(fileName);
	}
}

void TextureEditorWidget::updateTextureList() {
	d->allTextures = d->texturesService->listTextures(d->currentType);
	d->currentPage = 0;
	d->textureList->clear();
	loadTexturesPage();
}

void TextureEditorWidget::loadTexturesPage() {
	const int startIdx = d->currentPage * Private::kPageSize;
	if (startIdx >= d->allTextures.size()) {
		return; // Все данные уже загружены
	}

	const int endIdx = qMin(startIdx + Private::kPageSize, d->allTextures.size());

	// Блокируем сигналы, чтобы не вызывать onTextureItemSelected
	d->textureList->blockSignals(true);
	for (int i = startIdx; i < endIdx; ++i) {
		auto* item = new QListWidgetItem(d->allTextures[i]);
		d->textureList->addItem(item);
	}
	d->textureList->blockSignals(false);
	d->currentPage++;
}

void TextureEditorWidget::updatePreview(const QString& fileName) {
	const auto pixmap = d->texturesService->getTexture(fileName, d->currentType, "texture-editor");
	if (!pixmap.isNull()) {
		d->previewLabel->setPixmap(pixmap.scaled(
			d->previewLabel->size(),
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation));
	} else {
		d->previewLabel->setText("Не удалось загрузить\n" + fileName);
	}
}
