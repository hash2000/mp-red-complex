#pragma once
#include "ResourcesTool/widgets/animation/gl_widget.h"
#include "DataFormat/proto/frm.h"
#include "DataFormat/proto/pal.h"
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QWidget>
#include <QTimer>
#include <memory>
#include <cstdint>
#include <vector>

class FramesExplorerWidget : public QWidget{
public:
	FramesExplorerWidget(QWidget *parent = nullptr);

	void setup(const Proto::Animation &animation, const Proto::Pallete &pallete);

	uint8_t direction() const;
	void direction(uint8_t value);

  bool isPlaying() const;

	uint32_t frame() const;
	void frame(uint32_t value);

  qreal elapsed() const;
	void elapsed(qreal value);

  void setPlaybackRate(qreal rate); // 1.0 = normal, 2.0 = 2x, -1.0 = reverse
  void setFpsOverride(int fps);     // if 0 — use anim->fps

private slots:
    void onTimeout();

private:
	FrmGlWidget *_renderer;
	QTimer _timer;
	uint8_t _direction {0};
	uint32_t _frame {0};
	qreal _elapsed {0.0};
	qreal _playbackRate {1.0};
	uint32_t _fps {0};
};
