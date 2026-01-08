#pragma once
#include <QVariantMap>
#include <QWidget>

class WidgetsFactory {
public:
	WidgetsFactory(const QVariantMap& params);
	QWidget* create(QWidget* parent = nullptr) const;

private:
	const QVariantMap& _params;
};
