#pragma once
#include <QMainWindow>
#include <memory>

class DocumentArea;
class DocumentWidget;
class CommandContext;
class PropertiesPanel;


class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	/// @param buildDefaultPanels false — оболочка без панелей,
	/// они будут созданы декларативно (ClassManager + WidgetRegistry).
	explicit MainWindow(std::shared_ptr<CommandContext> context, QWidget* parent = nullptr,
		bool buildDefaultPanels = true);
	~MainWindow() override;

	DocumentArea* documentArea() const;
	void addDocument(DocumentWidget* doc);

	/// Панель свойств, связанная с активным документом.
	PropertiesPanel* propertiesPanel() const;

	/// Открыть файл в новом документе (используется панелью «Проводник»).
	void openFile(const QString& path);

	/// Регистрация панели справа (+ кнопка на Activity Bar + пункт меню «Вид»)
	void registerSidePanel(const QString& id, const QString& glyph,
		const QString& title, QWidget* panel, const QString& tooltip = QString());
	/// Регистрация нижней информационной панели
	void registerBottomPanel(const QString& id, const QString& glyph,
		const QString& title, QWidget* panel, const QString& tooltip = QString());
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
