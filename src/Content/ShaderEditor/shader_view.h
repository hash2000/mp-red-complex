#pragma once

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
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)
QT_FORWARD_DECLARE_CLASS(QOpenGLShader)
QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class ShaderView : public QOpenGLWidget, protected QOpenGLFunctions {
	Q_OBJECT

public:
	explicit ShaderView(const QVariantMap& params, QWidget* parent = nullptr);

	virtual ~ShaderView();

	void setVertexShaderCode(const QString& code);
	void setFragmentShaderCode(const QString& code);
	QString lastError() const;
	QString vertexCode() const;
	QString fragmentCode() const;
	void autoRender(bool set);

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;

private:
	bool compileAndLink();
	void setupQuadGeometry();

signals:
	void shaderReloaded(bool success);

private:
	int _viewWidth{ 800 };
	int _viewHeight{ 600 };
	QString _lastError;
	QString _vertexCode;
	QString _fragmentCode;
	QOpenGLShaderProgram _program;
	QOpenGLBuffer _vbo{ QOpenGLBuffer::VertexBuffer };
	QMatrix4x4 _proj;
	QTimer* _renderTimer;
	QElapsedTimer _timer;
};
