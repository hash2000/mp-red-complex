#include "Launcher/widgets/game_view/game_view.h"

GameView::GameView(const QColor& background, QWidget* parent)
: QOpenGLWidget(parent)
, _background(background) {
}

void GameView::onAction() {
}

QIcon GameView::icon() const {
	return QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew);
}

QString GameView::description() const {
	return "Game";
}

QString GameView::id() const {
	return "game";
}

ViewState GameView::viewState() const
{
	return ViewState::Game;
}

void GameView::initializeGL()
{
	initializeOpenGLFunctions();


}

void GameView::paintGL()
{
	glClearColor(_background.redF(), _background.greenF(), _background.blueF(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);






	update();
}
