#pragma once
#include <QWidget>
#include <memory>

class FetchApiWidget : public QWidget {
	Q_OBJECT
public:
	FetchApiWidget(QWidget* parent = nullptr);
	~FetchApiWidget() override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
