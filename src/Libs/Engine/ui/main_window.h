#pragma once
#include <QMainWindow>
#include <QStringList>
#include <functional>
#include <memory>
#include <optional>

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

	enum class WidgetKind { Document, BottomPanel, SidePanel };
	using WidgetFactory = std::function<QWidget*()>;

	/// Только GUI-поток. Пустой id и пустая фабрика недопустимы.
	/// При существующем id фабрика не вызывается, виджет активируется.
	/// При успехе окно принимает владение виджетом.
	/// type — xtype/класс, которым создан виджет (для списков и справки).
	QWidget* createWidget(const QString& id, WidgetKind kind, const QString& title,
		const QString& glyph, const WidgetFactory& factory, const QString& tooltip = QString(),
		const QString& type = QString());
	QWidget* widget(const QString& id) const;
	std::optional<WidgetKind> widgetKind(const QString& id) const;
	QString widgetTitle(const QString& id) const;
	QString widgetType(const QString& id) const;
	QStringList widgetIds() const;
	QStringList widgetIds(WidgetKind kind) const;
	bool showWidget(const QString& id);
	/// false: id отсутствует либо виджет отменил закрытие.
	bool closeWidget(const QString& id);

	DocumentArea* documentArea() const;
	void addDocument(DocumentWidget* doc);

	/// Панель свойств, связанная с активным документом.
	PropertiesPanel* propertiesPanel() const;

	/// Открыть файл в новом документе (используется панелью «Проводник»).
	void openFile(const QString& path);

	/// Регистрация панели справа (+ кнопка на Activity Bar + пункт меню «Вид»)
	void registerSidePanel(const QString& id, const QString& glyph,
		const QString& title, QWidget* panel, const QString& tooltip = QString(),
		const QString& type = QString());
	/// Регистрация нижней информационной панели
	void registerBottomPanel(const QString& id, const QString& glyph,
		const QString& title, QWidget* panel, const QString& tooltip = QString(),
		const QString& type = QString());
	void unregisterBottomPanel(const QString& id);
	void unregisterSidePanel(const QString& id);

	void showSidePanel(const QString& id);
	void hideSidePanel();
	void toggleSidePanel(const QString& id);
	void showBottomPanel(const QString& id);

	void buildDefaultPanels();

	void log(const QString& message);

protected:
	void closeEvent(QCloseEvent* e) override;

private:
	struct Private;
	std::unique_ptr<Private> d;
};
