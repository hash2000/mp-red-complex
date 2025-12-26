#pragma once
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QGroupBox>

class AtlasView;

class AtlasViewControlPanel : public QWidget {
	Q_OBJECT

public:
	explicit AtlasViewControlPanel(AtlasView* atlasView, QWidget* parent = nullptr);

private:
	void setupUi();
	void connectSignals();
	void updateDirectionComboBox();
	void updateDirectionInfo(int dirIndex);
	void updateFrameInfo(int dirIndex, int frameIndex);
	void clearFrameInfo();
	void updateFrameComboBox(int dirIndex);

private:
	AtlasView* _view;

	// UI controls
	QCheckBox* _rulerCheckBox = nullptr;
	QLabel* _directionLabel = nullptr;
	QComboBox* _directionComboBox = nullptr;
	QGroupBox* _directionInfoGroup = nullptr;
	QLabel* _dirFramesLabel = nullptr;
	QLabel* _dirShiftXLabel = nullptr;
	QLabel* _dirShiftYLabel = nullptr;

	QLabel* _frameLabel = nullptr;
	QComboBox* _frameComboBox = nullptr;
	QGroupBox* _frameInfoGroup = nullptr;
	QLabel* _frmWidthLabel = nullptr;
	QLabel* _frmHeightLabel = nullptr;
	QLabel* _frmOffsetXLabel = nullptr;
	QLabel* _frmOffsetYLabel = nullptr;
	QLabel* _frmSizeLabel = nullptr;
};
