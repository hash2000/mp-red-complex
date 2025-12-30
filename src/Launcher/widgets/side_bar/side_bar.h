#pragma once
#include "Launcher/widgets/side_bar/side_bar_action.h"
#include <QWidget>
#include <QAbstractButton>
#include <QHash>
#include <QVBoxLayout>
#include <QStackedWidget>

class SideBar : public QWidget {
	Q_OBJECT

public:
	explicit SideBar(QWidget* parent = nullptr);

	void addAction(SideBarAction* action);

	void setButtonVisible(const QString& id, bool visible);
	void setButtonChecked(const QString& id, bool check);

signals:
	void onAction(SideBarAction* action);

private:
	void uncheckAllButtons();

private:
	QHash<QString, QAbstractButton*> _buttons;
	QVBoxLayout* _layout;
	QStackedWidget* _widgetsStack;
};
