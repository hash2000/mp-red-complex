#pragma once
#include "Resources/resources.h"
#include <QWidget>
#include <QVariantMap>

class ShadersEditorView : public QWidget {
public:
	explicit ShadersEditorView(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget* parent = nullptr);

	virtual ~ShadersEditorView();


};
