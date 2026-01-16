#pragma once
#include "Resources/resources.h"
#include <QWidget>
#include <QHBoxLayout>
#include <memory>

class BaseTabWidget : public QWidget {
	Q_OBJECT
public:
	BaseTabWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* parent = nullptr);

	std::shared_ptr<DataStream> currentStream() const;

	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;

signals:
	void requestContainer(const QVariantMap& params);

protected:
	std::shared_ptr<Resources> _resources;
	QVariantMap _params;
};

