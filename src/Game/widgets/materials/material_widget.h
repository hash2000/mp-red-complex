#pragma once
#include "Graphics/widgets/base_opengl_widget.h"
#include <qtmetamacros.h>

class MaterialWidget : public BaseOpenGLWidget {
	Q_OBJECT

public:
	explicit MaterialWidget(QWidget* parent = nullptr);
	~MaterialWidget() override;

signals:
	void materialSelected(int materialId);

protected:
	void initializeGL() override;
	void paintGL() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
