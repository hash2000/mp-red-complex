#pragma once
#include "DataFormat/proto/pal.h"
#include "DataFormat/data_format/frm/atlas_builder.h"
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <memory>

class FrmGlWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT
public:
  FrmGlWidget(QWidget *parent = nullptr);
	virtual ~FrmGlWidget();

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

public slots:
  void setFrames(const std::vector<Proto::Frame> &frames, const Proto::Pallete &pallete);

private:
	void doUpdateAtlas();
	void doApplyCurrentFrame();

	void cleanup();

	void uploadFrames(const std::vector<Proto::Frame>& frames, const Proto::Pallete& pallete);
	void setCurrentFrame(int value);

private:
	bool _initialized{ false };

	std::unique_ptr<DataFormat::Frm::AtlasBuilder> _atlas;
	std::unique_ptr<QOpenGLTexture> _texture;
	QOpenGLShaderProgram _program;

	QOpenGLVertexArrayObject _vao;
	QOpenGLBuffer _vbo{ QOpenGLBuffer::VertexBuffer };

	bool _atlasPending{ false };
	bool _applyCurrentFrame{ false };

	int _currentFrame{ -1 };
	int _frameCount{ 0 };
	int _offsetX{ 0 };
	int _offsetY{ 0 };
	int _viewWidth{ 800 };
	int _viewHeight{ 600 };

};
