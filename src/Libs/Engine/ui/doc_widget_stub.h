#pragma once
#include "Libs/Engine/ui/doc_widget.h"

#include <QPointer>

/// Документ-обёртка для виджета, у которого нет собственного класса-документа.
/// Владеет содержимым: виджет помещается в layout обёртки, поэтому при
/// уничтожении документа корректно удаляется и его содержимое.
class DocumentWidgetStub : public DocumentWidget {
	Q_OBJECT
public:
	explicit DocumentWidgetStub(QWidget* content, QWidget* parent = nullptr);
	~DocumentWidgetStub() override;

	bool maybeClose() override;

private:
	QPointer<QWidget> _content;
};
