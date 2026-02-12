#pragma once
#include <QWidget>
#include <QRect>

class DropPreviewWidget : public QWidget {
	Q_OBJECT
public:
	explicit DropPreviewWidget(QWidget* parent = nullptr);
	~DropPreviewWidget() override;

	void showPreview(const QRect& rect, bool canPlace);
	void hidePreview();

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
