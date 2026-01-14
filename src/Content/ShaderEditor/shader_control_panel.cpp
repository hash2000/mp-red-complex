#include "Content/ShaderEditor/shader_control_panel.h"
#include "Content/ShaderEditor/shader_view.h"
#include <QFontDatabase>
#include <QSplitter>
#include <QVBoxLayout>

ShaderControlPanel::ShaderControlPanel(ShaderView* preview, QWidget* parent)
: QWidget(parent)
, _preview(preview)
, _tabs(new QTabWidget) {
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setPointSize(10);

	// Редакторы
	_vertexEdit = new QTextEdit;
	_vertexEdit->setFont(font);
	_vertexEdit->setPlainText(_preview->vertexCode());

	_fragmentEdit = new QTextEdit;
	_fragmentEdit->setFont(font);
	_fragmentEdit->setPlainText(_preview->fragmentCode());

	_errorLabel = new QTextEdit;
	_errorLabel->setStyleSheet("color: red; background: #222; padding: 5px;");
	_errorLabel->setReadOnly(true);

	_debounceTimer = new QTimer(this);
	_debounceTimer->setSingleShot(true);
	_debounceTimer->setInterval(1000);
	connect(_debounceTimer, &QTimer::timeout, this, [this]() {
		_preview->setVertexShaderCode(_vertexEdit->toPlainText());
		_preview->setFragmentShaderCode(_fragmentEdit->toPlainText());
		});

	// Сигналы редакторов
	connect(_vertexEdit, &QTextEdit::textChanged, this, &ShaderControlPanel::onVertexTextChanged);
	connect(_fragmentEdit, &QTextEdit::textChanged, this, &ShaderControlPanel::onFragmentTextChanged);
	connect(_preview, &ShaderView::shaderReloaded, this, &ShaderControlPanel::onShaderReloaded);

	_autoRenderCheckBox = new QCheckBox(tr("Auto render"));
	_autoRenderCheckBox->setChecked(false);
	_preview->autoRender(false);
	auto optionsLayput = new QVBoxLayout;
	optionsLayput->addWidget(_errorLabel);
	optionsLayput->addWidget(_autoRenderCheckBox);
	auto optionsWidget = new QWidget;
	optionsWidget->setLayout(optionsLayput);

	connect(_autoRenderCheckBox, &QCheckBox::toggled, [this](bool checked) {
		_preview->autoRender(checked);
		});

	// Layout
	_tabs->addTab(_vertexEdit, "Vertex");
	_tabs->addTab(_fragmentEdit, "Fragment");
	_tabs->addTab(optionsWidget, "Options");

	auto mainLayout = new QVBoxLayout;
	mainLayout->addWidget(_tabs);
	setLayout(mainLayout);
}

void ShaderControlPanel::onVertexTextChanged() {
	_debounceTimer->start();
}

void ShaderControlPanel::onFragmentTextChanged() {
	_debounceTimer->start();
}

void ShaderControlPanel::onShaderReloaded(bool success) {
	if (success) {
		_errorLabel->setText("✓ Shader compiled successfully");
		_errorLabel->setStyleSheet("color: lime; background: #222; padding: 5px;");
	}
	else {
		_errorLabel->setText(_preview->lastError());
		_errorLabel->setStyleSheet("color: red; background: #222; padding: 5px;");
	}
}

