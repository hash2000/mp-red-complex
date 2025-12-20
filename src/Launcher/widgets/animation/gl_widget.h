#pragma once
#include "Launcher/widgets/animation/frames_renderer_gl.h"
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <memory>

class FrmGlWidget : public QOpenGLWidget {
  Q_OBJECT
public:
  FrmGlWidget(std::unique_ptr<OpenGLFramesRenderer> backend, QWidget *parent = nullptr);

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

public slots:
  void setFrames(const std::vector<Proto::Frame> &frames, const Proto::Pallete &pallete);

private:
  std::unique_ptr<OpenGLFramesRenderer> _backend;
};
