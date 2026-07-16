#pragma once
#include <QWidget>
#include <memory>

class CommandContext;

class FetchApiWidget : public QWidget {
	Q_OBJECT
public:
	FetchApiWidget(CommandContext* commandContext, QWidget* parent = nullptr);
	~FetchApiWidget() override;

private slots:
	void onSendClicked();

private:
	class Private;
	std::unique_ptr<Private> d;
};
