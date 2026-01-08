#pragma once
#include <QWidget>
#include <QVariantMap>

class MapEditorView : public QWidget {
public:
	explicit MapEditorView(const QVariantMap& params, QWidget *parent = nullptr);

	virtual ~MapEditorView() = default;

};
