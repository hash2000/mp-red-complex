#pragma once
#include "Libs/Graphics/widgets/base_opengl_widget.h"
#include <qtmetamacros.h>

class MaterialWidget : public BaseOpenGLWidget {
	Q_OBJECT

public:
	explicit MaterialWidget(QWidget* parent = nullptr);
	~MaterialWidget() override;

private slots:
	void onBerginFrame();
	void onInitializeContext();

private:
	class Private;
	std::unique_ptr<Private> d;
};
