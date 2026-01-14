#pragma once
#include <QWidget>
#include <QHBoxLayout>

class BaseTabWidget : public QWidget {
public:
	BaseTabWidget(QWidget* parent = nullptr);
};


class BaseTabWidgetWrapper : public BaseTabWidget {
public:
	BaseTabWidgetWrapper(QWidget* inner, QWidget* parent = nullptr) : BaseTabWidget(parent) {
		auto layout = new QHBoxLayout(this);
		layout->addWidget(inner);
		setLayout(layout);
	}
};
