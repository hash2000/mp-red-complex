#pragma once
#include <QWidget>
#include <memory>

class CommandProcessor;
class CommandContext;
class ApplicationController;

class CommandConsole : public QWidget {
	Q_OBJECT
public:
	explicit CommandConsole(ApplicationController* controller, QWidget* parent = nullptr);
	~CommandConsole() override;

	// Публичный интерфейс для внешнего управления
	void showConsole();
	void hideConsole();
	bool isVisible() const;

	// Добавление сообщения в историю (извне)
	void appendMessage(const QString& message, const QString& styleClass = "info");

protected:
	void showEvent(QShowEvent* event) override;
	void hideEvent(QHideEvent* event) override;
	bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
	void onCommandSubmitted();
	void onCommandTextChanged(const QString& text);
	void onHistoryUp();
	void onHistoryDown();
	void onCompleterActivated(const QString& text);
	void onOutputRequested(const QString& message, const QString& styleClass);

private:
	void setupUi();
	void setupCompleter();
	void addToHistory(const QString& command);
	QString getHistoryEntry(int offset);
	void executeCommand(const QString& command);
	void focusInput();

	// Стилизация вывода
	void setupOutputStyling();

private:
	class Private;
	std::unique_ptr<Private> d;
};
