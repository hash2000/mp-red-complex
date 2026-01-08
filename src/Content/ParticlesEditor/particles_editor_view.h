#pragma once
#include <QWidget>
#include <QVariantMap>

class ParticlesEditorView : public QWidget {
public:
	explicit ParticlesEditorView(const QVariantMap& params, QWidget* parent = nullptr);

	virtual ~ParticlesEditorView() = default;

};
