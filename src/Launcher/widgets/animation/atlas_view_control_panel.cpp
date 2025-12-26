#include "Launcher/widgets/animation/atlas_view_control_panel.h"
#include "Launcher/widgets/animation/atlas_view.h"

AtlasViewControlPanel::AtlasViewControlPanel(AtlasView* atlasView, QWidget* parent)
  : QWidget(parent)
  , _view(atlasView) {
  setupUi();
  connectSignals();
  updateDirectionComboBox();
  updateFrameInfo(0, 0);
}

void AtlasViewControlPanel::setupUi() {
  // --- Чекбокс линейки ---
  _rulerCheckBox = new QCheckBox(tr("Show Ruler"), this);
  _rulerCheckBox->setChecked(true);

  // --- Выбор направления ---
  _directionLabel = new QLabel(tr("Direction:"), this);
  _directionComboBox = new QComboBox(this);

  // --- Информация по направлению ---
  _directionInfoGroup = new QGroupBox(tr("Direction Info"), this);
  auto* dirLayout = new QFormLayout;
  _dirFramesLabel = new QLabel("0", this);
  _dirShiftXLabel = new QLabel("0", this);
  _dirShiftYLabel = new QLabel("0", this);
  dirLayout->addRow(tr("Frames:"), _dirFramesLabel);
  dirLayout->addRow(tr("Shift X:"), _dirShiftXLabel);
  dirLayout->addRow(tr("Shift Y:"), _dirShiftYLabel);
  _directionInfoGroup->setLayout(dirLayout);

  // --- Выбор фрейма (только для информации) ---
  _frameLabel = new QLabel(tr("Frame:"), this);
  _frameComboBox = new QComboBox(this);
  _frameComboBox->setEnabled(false); // только для просмотра

  // --- Информация по фрейму ---
  _frameInfoGroup = new QGroupBox(tr("Frame Info"), this);
  auto* frmLayout = new QFormLayout;
  _frmWidthLabel = new QLabel("0", this);
  _frmHeightLabel = new QLabel("0", this);
  _frmOffsetXLabel = new QLabel("0", this);
  _frmOffsetYLabel = new QLabel("0", this);
  _frmSizeLabel = new QLabel("0", this);
  frmLayout->addRow(tr("Width:"), _frmWidthLabel);
  frmLayout->addRow(tr("Height:"), _frmHeightLabel);
  frmLayout->addRow(tr("Offset X:"), _frmOffsetXLabel);
  frmLayout->addRow(tr("Offset Y:"), _frmOffsetYLabel);
  frmLayout->addRow(tr("Data Size:"), _frmSizeLabel);
  _frameInfoGroup->setLayout(frmLayout);

  // --- Общий layout ---
  auto* mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(_rulerCheckBox);

  auto* dirLayout2 = new QHBoxLayout;
  dirLayout2->addWidget(_directionLabel);
  dirLayout2->addWidget(_directionComboBox, 1);
  mainLayout->addLayout(dirLayout2);
  mainLayout->addWidget(_directionInfoGroup);

  auto* frmLayout2 = new QHBoxLayout;
  frmLayout2->addWidget(_frameLabel);
  frmLayout2->addWidget(_frameComboBox, 1);
  mainLayout->addLayout(frmLayout2);
  mainLayout->addWidget(_frameInfoGroup);

  mainLayout->addStretch();
  setLayout(mainLayout);
}

void AtlasViewControlPanel::connectSignals() {

  connect(_rulerCheckBox, &QCheckBox::toggled, [this](bool checked) {
		_view->showRuler(checked);
	});

  connect(_directionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
	  if (index >= 0 && static_cast<size_t>(index) < _view->directionsCount()) {
			_view->setDirection(static_cast<size_t>(index));
			updateDirectionInfo(index);
			updateFrameComboBox(index);
			if (_frameComboBox->count() > 0) {
				updateFrameInfo(index, 0);
			}
	  }
	});

  connect(_frameComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int frameIndex) {
	  if (frameIndex >= 0) {
			const int dirIndex = _directionComboBox->currentIndex();
			if (dirIndex >= 0) {
				updateFrameInfo(dirIndex, frameIndex);
			}
	  }
	});
}

void AtlasViewControlPanel::updateDirectionComboBox() {
  _directionComboBox->clear();
  const size_t count = _view->directionsCount();

  for (size_t i = 0; i < count; ++i) {
		_directionComboBox->addItem(QString::number(i));
  }

  if (count > 0) {
		_directionComboBox->setCurrentIndex(0);
		updateDirectionInfo(0);
		updateFrameComboBox(0);
  }
}

void AtlasViewControlPanel::updateDirectionInfo(int dirIndex) {
  if (!_view || !_view->animation()) {
		_dirFramesLabel->setText("–");
		_dirShiftXLabel->setText("–");
		_dirShiftYLabel->setText("–");
		return;
  }

  const auto& anim = *_view->animation();
  if (dirIndex < 0 || static_cast<size_t>(dirIndex) >= anim.directions.size()) {
		return;
  }

  const auto& dir = anim.directions[dirIndex];
  _dirFramesLabel->setText(QString::number(dir.frames.size()));
  _dirShiftXLabel->setText(QString::number(dir.shiftX));
  _dirShiftYLabel->setText(QString::number(dir.shiftY));
}

void AtlasViewControlPanel::updateFrameComboBox(int dirIndex) {
  _frameComboBox->clear();
	if (!_view || !_view->animation()) {
		return;
	}

  const auto& anim = *_view->animation();
	if (dirIndex < 0 || static_cast<size_t>(dirIndex) >= anim.directions.size()) {
		return;
	}

  const auto& dir = anim.directions[dirIndex];
  for (size_t i = 0; i < dir.frames.size(); ++i) {
		_frameComboBox->addItem(QString::number(i));
  }

  if (!dir.frames.empty()) {
		_frameComboBox->setCurrentIndex(0);
  }
}

void AtlasViewControlPanel::updateFrameInfo(int dirIndex, int frameIndex) {
  if (!_view || !_view->animation()) {
		clearFrameInfo();
		return;
  }

  const auto& anim = *_view->animation();
  if (dirIndex < 0 || static_cast<size_t>(dirIndex) >= anim.directions.size()) {
		clearFrameInfo();
		return;
  }

  const auto& dir = anim.directions[dirIndex];
  if (frameIndex < 0 || static_cast<size_t>(frameIndex) >= dir.frames.size()) {
		clearFrameInfo();
		return;
  }

  const auto& frm = dir.frames[frameIndex];
  _frmWidthLabel->setText(QString::number(frm.width));
  _frmHeightLabel->setText(QString::number(frm.height));
  _frmOffsetXLabel->setText(QString::number(frm.offsetX));
  _frmOffsetYLabel->setText(QString::number(frm.offsetY));
  _frmSizeLabel->setText(QString::number(frm.frameSize));
}

void AtlasViewControlPanel::clearFrameInfo() {
  _frmWidthLabel->setText("–");
  _frmHeightLabel->setText("–");
  _frmOffsetXLabel->setText("–");
  _frmOffsetYLabel->setText("–");
  _frmSizeLabel->setText("–");
}
