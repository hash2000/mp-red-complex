#pragma once
#include <QWidget>
#include <memory>

/// Вспомогательный виджет для инициализации OpenGL контекста
/// Предотвращает мерцание при первом создании QOpenGLWidget в MDI-области
class WarmupWidget : public QWidget {
	Q_OBJECT
public:
	explicit WarmupWidget(QWidget* parent = nullptr);
	~WarmupWidget() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
