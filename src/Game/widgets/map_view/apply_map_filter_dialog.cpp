#include "Game/widgets/map_view/apply_map_filter_dialog.h"
#include "Graphics/textures/uploaded_texture.h"
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QDialogButtonBox>

class ApplyMapAtlasDialog::Private {
public:

	QComboBox* textureFilterCombo = nullptr;
	QCheckBox* useMipMapCheckBox = nullptr;
	QCheckBox* useMipMapSmoothingCheckBox = nullptr;

	void setupUI(std::optional<UploadedTextureProperties> settings, ApplyMapAtlasDialog* q) {
		q->setWindowTitle(tr("Apply Map Atlas Settings"));

		auto* mainLayout = new QVBoxLayout(q);
		mainLayout->setContentsMargins(10, 10, 10, 10);

		auto* formLayout = new QFormLayout();
		formLayout->setHorizontalSpacing(15);
		formLayout->setVerticalSpacing(10);

		// 1. Комбобокс фильтра
		textureFilterCombo = new QComboBox(q);
		textureFilterCombo->addItem(tr("Linear"), static_cast<int>(TextureFilter::Linear));
		textureFilterCombo->addItem(tr("Nearest"), static_cast<int>(TextureFilter::Nearest));
		formLayout->addRow(tr("Texture Filter:"), textureFilterCombo);

		// 2. Чекбокс MipMap
		useMipMapCheckBox = new QCheckBox(tr("Use MipMap"), q);
		formLayout->addRow(tr("MipMap:"), useMipMapCheckBox);

		// 3. Чекбокс сглаживания
		useMipMapSmoothingCheckBox = new QCheckBox(tr("Use MipMap Smoothing"), q);
		useMipMapSmoothingCheckBox->setEnabled(false);
		formLayout->addRow(tr("MipMap Smoothing:"), useMipMapSmoothingCheckBox);

		mainLayout->addLayout(formLayout);
		mainLayout->addStretch(1);

		// Связывание состояния чекбоксов без использования слотов Q_OBJECT
		QObject::connect(useMipMapCheckBox, &QCheckBox::toggled,
			useMipMapSmoothingCheckBox, &QCheckBox::setEnabled);

		// Кнопки OK / Cancel
		auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q);
		QObject::connect(buttonBox, &QDialogButtonBox::accepted, q, &QDialog::accept);
		QObject::connect(buttonBox, &QDialogButtonBox::rejected, q, &QDialog::reject);
		mainLayout->addWidget(buttonBox);

		if (settings.has_value()) {
			textureFilterCombo->setCurrentIndex(static_cast<int>(settings->textureFilter));
			useMipMapCheckBox->setChecked(settings->useMipMaps);
			useMipMapSmoothingCheckBox->setChecked(settings->useMipMapsSmoothing);
		}
	}
};

ApplyMapAtlasDialog::ApplyMapAtlasDialog(std::optional<UploadedTextureProperties> settings, QWidget* parent)
	: QDialog(parent)
	, d(std::make_unique<Private>())
{
	d->setupUI(settings, this);
}

// Деструктор должен быть определён здесь, где тип `Private` уже завершён.
ApplyMapAtlasDialog::~ApplyMapAtlasDialog() = default;


UploadedTextureProperties ApplyMapAtlasDialog::settings() const {
	return {
		d->useMipMapCheckBox->isChecked(),
		d->useMipMapSmoothingCheckBox->isChecked(),
		static_cast<TextureFilter>(d->textureFilterCombo->currentData().toInt()),
	};
}
