#pragma once
#include <QMainWindow>
#include <memory>

class DocumentArea;
class DocumentWidget;

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

	DocumentArea* documentArea() const;
	void addDocument(DocumentWidget* doc);

	/// Регистрация панели справа (+ кнопка на Activity Bar + пункт меню «Вид»)
	void registerSidePanel(const QString& id, const QString& glyph,
		const QString& title, QWidget* panel);
	/// Регистрация нижней информационной панели
	void registerBottomPanel(const QString& id, const QString& glyph,
		const QString& title, QWidget* panel);
	void unregisterBottomPanel(const QString& id);

	void showSidePanel(const QString& id);
	void hideSidePanel();
	void toggleSidePanel(const QString& id);
	void showBottomPanel(const QString& id);

	void log(const QString& message);

protected:
	void closeEvent(QCloseEvent* e) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
