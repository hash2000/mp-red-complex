#pragma once
#include <QWidget>
#include <memory>

class CommandProcessor;
class CommandContext;
class CommandController;

class CommandConsole : public QWidget {
	Q_OBJECT
public:
	explicit CommandConsole(CommandController* controller, QWidget* parent = nullptr);
	~CommandConsole() override;

	// Публичный интерфейс для внешнего управления
	void showConsole();
	void hideConsole();
	bool isVisible() const;

	// Добавление сообщения в историю (извне)
	void appendMessage(const QString& message, const QString& styleClass);

protected:
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
	void onHistoryUp();
	void onHistoryDown();
	void onOutputRequested(const QString& message, const QString& styleClass, const QString& type);
	void onOutputClear();

private:
	class Private;
	std::unique_ptr<Private> d;
};
