#include "ResourcesTool/widgets/animation/frames_explorer_widget.h"
#include "DataFormat/data_format/frm/atlas_builder.h"
#include <QVBoxLayout>

FramesExplorerWidget::FramesExplorerWidget(QWidget *parent)
: QWidget(parent) {
	setObjectName("FramesExplorerWidget");
	_renderer = new FrmGlWidget(this);

	auto layout = new QVBoxLayout(this);
	setLayout(layout);

	layout->addWidget(_renderer);

	_timer.setTimerType(Qt::PreciseTimer);
  connect(&_timer, &QTimer::timeout, this, &FramesExplorerWidget::onTimeout);
}

void FramesExplorerWidget::onTimeout() {

}

void FramesExplorerWidget::setup(const Proto::Animation &animation, const Proto::Pallete &pallete) {
	_renderer->setFrames(animation.directions[_direction].frames, pallete);
}

uint8_t FramesExplorerWidget::direction() const {
	return _direction;
}

void FramesExplorerWidget::direction(uint8_t value) {
	_direction = value;
}

bool FramesExplorerWidget::isPlaying() const {
	return _timer.isActive();
}

uint32_t FramesExplorerWidget::frame() const {
	return _frame;
}

void FramesExplorerWidget::frame(uint32_t value) {
	_frame = value;
}

qreal FramesExplorerWidget::elapsed() const {
	return _elapsed;
}

void FramesExplorerWidget::elapsed(qreal value) {
	_elapsed = value;
}

void FramesExplorerWidget::setPlaybackRate(qreal rate) {
	_playbackRate = rate;
}

void FramesExplorerWidget::setFpsOverride(int fps) {
	_fps = fps;
}
