#pragma once
#include <QWidget>
#include <QVariantMap>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>

#include <QVector3D>
#include <QMatrix4x4>

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShader)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class ParticlesEditorView : public QOpenGLWidget, protected QOpenGLFunctions {
public:
	explicit ParticlesEditorView(const QVariantMap& params, QWidget* parent = nullptr);

	virtual ~ParticlesEditorView();

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

private:
	int _viewWidth{ 800 };
	int _viewHeight{ 600 };

};
