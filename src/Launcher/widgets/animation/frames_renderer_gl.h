#pragma once
#include "DataFormat/proto/pal.h"
#include "DataFormat/data_format/frm/atlas_builder.h"
#include "Launcher/widgets/animation/frames_renderer.h"
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <memory>

class OpenGLFramesRenderer : public IFrameRenderer, protected QOpenGLFunctions {
public:
	explicit OpenGLFramesRenderer();
  virtual ~OpenGLFramesRenderer();

	void initialize() override;

	void uploadFrames(const std::vector<Proto::Frame> &frames, const Proto::Pallete &pallete) override;
	void setCurrentFrame(int value) override;

  void render() override;
  void resize(int width, int height) override;
  QString name() const override;
  bool initialized() const override;

private:
	void doUpdateAtlas();
	void doApplyCurrentFrame();

	void cleanup();

private:
  bool _initialized{false};

	std::unique_ptr<DataFormat::Frm::AtlasBuilder> _atlas;
  std::unique_ptr<QOpenGLTexture> _texture;
  QOpenGLShaderProgram _program;

	QOpenGLVertexArrayObject _vao;
	QOpenGLBuffer _vbo{QOpenGLBuffer::VertexBuffer};

	bool _atlasPending {false};
	bool _applyCurrentFrame {false};

	int _currentFrame {-1};
	int _frameCount {0};
  int _offsetX {0};
	int _offsetY {0};
  int _viewWidth {800};
	int _viewHeight {600};
};
