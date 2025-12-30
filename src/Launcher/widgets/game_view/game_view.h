#pragma once
#include "Launcher/widgets/side_bar/side_bar_action.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>

class GameView : public QOpenGLWidget, protected QOpenGLFunctions, public SideBarAction {
public:
	GameView(const QColor& background, QWidget* parent = nullptr);

	void onAction() override;
	virtual QIcon icon() const override;
	virtual QString description() const override;
	QString id() const override;
	ViewState viewState() const override;

protected:
	void paintGL() override;
	void initializeGL() override;

private:
	QColor _background;
};
