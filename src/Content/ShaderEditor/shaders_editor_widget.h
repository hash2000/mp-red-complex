#pragma once
#include "Resources/resources.h"
#include <QWidget>
#include <QVariantMap>

class ShadersEditorWidget : public QWidget {
public:
	explicit ShadersEditorWidget(std::shared_ptr<Resources> resources, const QVariantMap& params,
		QWidget* parent = nullptr);

	virtual ~ShadersEditorWidget();
};
