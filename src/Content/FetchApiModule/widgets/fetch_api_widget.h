#pragma once
#include <QWidget>
#include <memory>

class FetchApiService;

class FetchApiWidget : public QWidget {
	Q_OBJECT
public:
	FetchApiWidget(FetchApiService* fetchApiService, QWidget* parent = nullptr);
	~FetchApiWidget() override;

private slots:
	void onSendClicked();

private:
	class Private;
	std::unique_ptr<Private> d;
};
