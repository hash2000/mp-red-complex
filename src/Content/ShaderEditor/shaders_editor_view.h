#pragma once
#include <QWidget>
#include <QVariantMap>

class ShadersEditorView : public QWidget {
public:
	explicit ShadersEditorView(const QVariantMap& params, QWidget* parent = nullptr);

	virtual ~ShadersEditorView();


};
