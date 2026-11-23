#pragma once
#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include <QVariant>
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
	/// Зоны оболочки, создаваемые при построении.
	/// Выключенная зона (yaml: bottombar/sidebar delete: true) не создаётся:
	/// попытка добавить в неё виджет пишет qWarning() и отклоняется.
	struct Options {
		bool bottomPanel = true;   ///< bottombar
		bool sidePanel = true;     ///< sidebar

		/// Разбирает декларативные параметры построения виджета
		/// (WidgetBuildHandler::options): ключи "bottombar.delete"/"sidebar.delete",
		/// по умолчанию false.
		static Options fromBuildOptions(const QMap<QString, QVariant>& values);
	};

	/// @param buildDefaultPanels false — оболочка без панелей,
	/// они будут созданы декларативно (ClassManager + WidgetRegistry).
	explicit MainWindow(std::shared_ptr<CommandContext> context, QWidget* parent = nullptr,
		bool buildDefaultPanels = true);
	/// @param options выключенные зоны оболочки из описания приложения.
	MainWindow(std::shared_ptr<CommandContext> context, QWidget* parent, bool buildDefaultPanels,
		const Options& options);
	~MainWindow() override;

	enum class WidgetKind { Document, BottomPanel, SidePanel };
	using WidgetFactory = std::function<QWidget*()>;

	/// Только GUI-поток. Пустой id и пустая фабрика недопустимы.
	/// При существующем id фабрика не вызывается, виджет активируется.
	/// При успехе окно принимает владение виджетом.
	/// type — xtype/класс, которым создан виджет (для списков и справки).
	/// creatorContext — контекст, создающий документ: новый документ получает
	/// контекст `{creator}.documents.{id}`; nullptr — контекст вьюпорта.
	/// variables — начальные переменные контекста документа: записываются
	/// до привязки контекста, поэтому документ читает их при инициализации.
	/// Любой виджет любой зоны является документом (оборачивается в
	/// DocumentWidget), поэтому связан с остальными через CommandContext.
	QWidget* createWidget(const QString& id, WidgetKind kind, const QString& title,
		const QString& glyph, const WidgetFactory& factory, const QString& tooltip = QString(),
		const QString& type = QString(),
		const std::shared_ptr<CommandContext>& creatorContext = std::shared_ptr<CommandContext>(),
		const QVariantMap& variables = QVariantMap());
	QWidget* widget(const QString& id) const;
	/// Документ виджета (обёртка DocumentWidget) либо nullptr.
	DocumentWidget* document(const QString& id) const;
	/// Контекст документа по id виджета.
	std::shared_ptr<CommandContext> documentContext(const QString& id) const;
	/// Идентификатор активного документа (в любой зоне) либо пустая строка.
	QString activeDocumentId() const;
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

	/// Показать документ в панели «Свойства» и раскрыть группу его свойств.
	/// false — панель свойств недоступна.
	bool showDocumentProperties(DocumentWidget* doc, const QString& groupId = QString());

	/// Открыть файл в новом документе (используется панелью «Проводник»).
	void openFile(const QString& path);

	/// Создана ли зона (выключенные описанием приложения зоны виджетов не принимают).
	/// Для WidgetKind::Document всегда true.
	bool isZoneEnabled(WidgetKind kind) const;

	/// Панель открытых документов слева (yaml: content.documents.showTabs).
	void setDocumentTabsVisible(bool visible);
	bool isDocumentTabsVisible() const;

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
