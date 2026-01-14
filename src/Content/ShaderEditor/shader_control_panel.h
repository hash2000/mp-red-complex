#pragma once
#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QTimer>
#include <QTabWidget>
#include <QCheckBox>

class ShaderView;

class ShaderControlPanel : public QWidget {
	Q_OBJECT
public:
	ShaderControlPanel(ShaderView *preview, QWidget* parent = nullptr);

private slots:
	void onVertexTextChanged();
	void onFragmentTextChanged();
	void onShaderReloaded(bool success);

private:
	ShaderView* _preview;
	QTextEdit* _vertexEdit;
	QTextEdit* _fragmentEdit;
	QTextEdit* _errorLabel;
	QTimer* _debounceTimer;
	QTabWidget* _tabs;
	QCheckBox* _autoRenderCheckBox;
};
