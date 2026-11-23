#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/activity_bar.h"
#include "Libs/Engine/ui/bottom_panel.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/side_host_panel.h"
#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/ui/main_window/new_document_dialog.h"
#include "Libs/Engine/ui/main_window/system_options_document.h"
#include "Libs/Engine/ui/text_document.h"
#include "Libs/Engine/ui/doc_widget_stub.h"
#include "Libs/Engine/ui/command_console.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPointer>
#include <QScopeGuard>
#include <QSet>
#include <QSplitter>
#include <QThread>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QTableWidget>
#include <QTime>
#include <QTreeView>

#include <exception>

namespace {
QWidget* makePlaceholder(const QString& text) {
	auto* l = new QLabel(text);
	l->setObjectName(QStringLiteral("EmptyHint"));
	l->setAlignment(Qt::AlignCenter);
	l->setWordWrap(true);
	return l;
}
} // namespace

struct MainWindow::Private {
	explicit Private(MainWindow* qq) : q(qq) {}

	MainWindow* q;
	Options options;

	DocumentArea* area = nullptr;
	DocumentTabsPanel* tabs = nullptr;
	SideHostPanel* side = nullptr;
	ActivityBar* activity = nullptr;
	BottomPanel* bottom = nullptr;
	QSplitter* mainSplitter = nullptr;    // [tabs | center | side]
	QSplitter* centerSplitter = nullptr;  // [area / bottom]
	QPlainTextEdit* output = nullptr;
	QLabel* statusDoc = nullptr;
	QMenu* viewMenu = nullptr;
	QAction* sidePanelsSeparator = nullptr;
	QAction* pinAction = nullptr;

	QHash<QString, QAction*> bottomActions;
	QHash<QString, QAction*> sideActions;
	struct Entry {
		WidgetKind kind;
		QPointer<QWidget> widget;
		QPointer<DocumentWidget> document;
		DocumentWidget* documentIdentity = nullptr; // только сравнение адресов
		quint64 serial;
		QString title;
		QString type;
	};
	QHash<QString, Entry> entries;
	QSet<QString> creating;
	quint64 nextSerial = 0;
	QPointer<CommandController> controller;

	/// Активный документ любой зоны; нужен для documentActivated/Deactivated.
	QString activeDocument;
	QPointer<DocumentWidget> activeDocumentWidget;

	void activateDocument(const QString& id);
	void deactivateDocument();
	QString idOfDocument(DocumentWidget* doc) const;

	std::shared_ptr<CommandContext> context;
	void track(const QString& id, WidgetKind kind, QWidget* widget, const QString& title,
		const QString& type, DocumentWidget* document = nullptr);
	void forgetDocument(DocumentWidget* doc);

	int untitledCounter = 0;
	int lastSideWidth = 280;
	int lastBottomHeight = 220;
	int lastTabsWidth = 220;
	bool documentTabsVisible = true;

	void buildUi();
	void buildMenus();
	void wireSignals();

	/// Панель «Свойства» среди созданных виджетов (панель может быть
	/// зарегистрирована декларативно, поэтому её нельзя хранить заранее).
	PropertiesPanel* findPropertiesPanel() const;
	/// Идентификатор виджета, в котором находится панель свойств.
	QString propertiesPanelEntryId() const;

	void newDocument();
	/// Создаёт документ зарегистрированного типа (повторный id активирует
	/// существующий). Возвращает документ либо nullptr.
	QWidget* createDocumentOfType(const QString& type, const QString& requestedId,
		const QString& title, const QString& glyph,
		const QVariantMap& variables = QVariantMap());
	/// Открывает документ системных настроек (роль DocumentRule::SystemOptions).
	void openSystemOptions();
	void openFile(const QString& path);
	void toggleBottom();
	void updateWindowTitle(DocumentWidget* doc);
};

void MainWindow::Private::buildUi() {
	auto* central = new QWidget(q);
	auto* h = new QHBoxLayout(central);
	h->setContentsMargins(0, 0, 0, 0);
	h->setSpacing(0);

	mainSplitter = new QSplitter(Qt::Horizontal, central);
	mainSplitter->setHandleWidth(1);
	mainSplitter->setChildrenCollapsible(false);

	// левая панель открытых документов; из контекста системных настроек
	// она читает правило сортировки/группировки вкладок
	tabs = new DocumentTabsPanel(context);

	centerSplitter = new QSplitter(Qt::Vertical);
	centerSplitter->setHandleWidth(1);
	centerSplitter->setChildrenCollapsible(false);

	// центральная панель, область открытого документа
	area = new DocumentArea;

	// информационная панель снизу: только если зона не удалена описанием
	centerSplitter->addWidget(area);
	if (options.bottomPanel) {
		bottom = new BottomPanel;
		centerSplitter->addWidget(bottom);
		centerSplitter->setStretchFactor(1, 0);
	}
	centerSplitter->setStretchFactor(0, 1);

	// область панелей справа: только если зона не удалена описанием
	if (options.sidePanel) {
		side = new SideHostPanel;
	}

	mainSplitter->addWidget(tabs);
	mainSplitter->addWidget(centerSplitter);
	if (side) {
		mainSplitter->addWidget(side);
		mainSplitter->setSizes({ lastTabsWidth, 900, lastSideWidth });
	}
	else {
		mainSplitter->setSizes({ lastTabsWidth, 900 });
	}
	mainSplitter->setStretchFactor(0, 0);
	mainSplitter->setStretchFactor(1, 1);
	if (side) {
		mainSplitter->setStretchFactor(2, 0);
	}
	centerSplitter->setSizes(bottom ? QList<int>{ 600, lastBottomHeight } : QList<int>{ 600 });

	// область кнопок панелей справа: без панелей справа она не нужна
	if (side) {
		activity = new ActivityBar(central);
	}

	h->addWidget(mainSplitter, 1);
	if (activity) {
		h->addWidget(activity);
	}
	q->setCentralWidget(central);

	// Статус-бар
	q->statusBar()->addWidget(new QLabel(MainWindow::tr("Готово")));
	statusDoc = new QLabel;
	q->statusBar()->addPermanentWidget(statusDoc);

	updateWindowTitle(nullptr);
}

void MainWindow::Private::buildMenus() {
	auto* file = q->menuBar()->addMenu(MainWindow::tr("&Файл"));
	file->addAction(MainWindow::tr("Новый документ"), QKeySequence::New, q, [this] {
		newDocument();
	});
	file->addAction(MainWindow::tr("Сохранить"), QKeySequence::Save, q, [this] {
		if (auto* doc = area->currentDocument())
			doc->save();
	});
	file->addAction(MainWindow::tr("Закрыть документ"), QKeySequence(Qt::CTRL | Qt::Key_W), q, [this] {
		area->closeDocument(area->currentDocument());
	});
	file->addSeparator();
	file->addAction(MainWindow::tr("Выход"), QKeySequence::Quit, q, &QWidget::close);

	viewMenu = q->menuBar()->addMenu(MainWindow::tr("&Вид"));
	sidePanelsSeparator = viewMenu->addSeparator();   // сюда будут вставляться панели
	if (bottom) {
		viewMenu->addAction(MainWindow::tr("Нижняя панель"), QKeySequence(Qt::CTRL | Qt::Key_J),
			q, [this] { toggleBottom(); });
	}
	if (activity) {
		viewMenu->addSeparator();
		pinAction = viewMenu->addAction(MainWindow::tr("📌 Закрепить панель кнопок"));
		pinAction->setCheckable(true);
		pinAction->setChecked(true);
		QObject::connect(pinAction, &QAction::toggled, side, [this](bool pinned) {
			activity->setAutoHide(!pinned);
			pinAction->setChecked(pinned);
		});
	}
}

void MainWindow::Private::wireSignals() {
	// Левая панель вкладок -> область документов
	QObject::connect(tabs, &DocumentTabsPanel::activateRequested, area, &DocumentArea::setCurrentDocument);
	QObject::connect(tabs, &DocumentTabsPanel::closeRequested, area, &DocumentArea::closeDocument);
	QObject::connect(tabs, &DocumentTabsPanel::newDocumentRequested, q, [this] { newDocument(); });
	QObject::connect(tabs, &DocumentTabsPanel::propertiesRequested, q, [this](DocumentWidget* doc) {
		q->showDocumentProperties(doc);
	});

	// Область документов -> левая панель, свойства, статус
	QObject::connect(area, &DocumentArea::documentAdded, q, [this](DocumentWidget* doc) {
		tabs->addDocument(doc);
		QObject::connect(doc, &DocumentWidget::titleChanged, q, [this, doc] {
			if (area->currentDocument() == doc)
				updateWindowTitle(doc);
		});
		q->log(MainWindow::tr("Открыт документ: %1").arg(doc->title()));
	});
	QObject::connect(area, &DocumentArea::documentAboutToClose, q, [this](DocumentWidget* doc) {
		forgetDocument(doc);
		tabs->removeDocument(doc);
		if (auto* properties = q->propertiesPanel(); properties && properties->document() == doc)
			properties->setDocument(nullptr);
		q->log(MainWindow::tr("Закрыт документ: %1").arg(doc->title()));
	});
	QObject::connect(area, &DocumentArea::documentDestroyed, q, [this](DocumentWidget* doc) {
		forgetDocument(doc);
		tabs->removeDocument(doc);
	});
	QObject::connect(area, &DocumentArea::currentDocumentChanged, q, [this](DocumentWidget* doc) {
		tabs->setActiveDocument(doc);
		if (auto* properties = q->propertiesPanel())
			properties->setDocument(doc);   // «Свойства» следуют за активным документом
		updateWindowTitle(doc);
		activateDocument(idOfDocument(doc));
	});

	// Правая сторона
	if (activity) {
		QObject::connect(activity, &ActivityBar::buttonClicked, q, [this](const QString& id) {
			if (id == QLatin1String("settings")) {
				// Кнопка «Настройки» открывает документ системных настроек:
				// он работает в контексте `.options` и может менять глобальные
				// системные параметры.
				openSystemOptions();
				return;
			}
			q->toggleSidePanel(id);
		});
	}
	if (side) {
		QObject::connect(side, &SideHostPanel::closeRequested, q, [this] { q->hideSidePanel(); });
	}

	if (bottom) {
		// Нижняя панель
		QObject::connect(bottom, &BottomPanel::closeRequested, bottom, &QWidget::hide);
		QObject::connect(bottom, &BottomPanel::collapsedChanged, q, [this](bool collapsed) {
			const auto s = centerSplitter->sizes();
			const auto total = s.value(0) + s.value(1);
			const auto height = collapsed ? QWIDGETSIZE_MAX : lastBottomHeight;
			const int panelHeight = collapsed
				? bottom->height() // уже равна высоте заголовка
				: lastBottomHeight; // вернуть прежнюю высоту

			centerSplitter->setSizes({ total - panelHeight, panelHeight });
		});
	}

	// Запоминаем размеры, выставленные пользователем
	QObject::connect(mainSplitter, &QSplitter::splitterMoved, q, [this] {
		if (side && side->isVisible())
			lastSideWidth = side->width();
	});
	QObject::connect(centerSplitter, &QSplitter::splitterMoved, q, [this] {
		if (bottom && bottom->isVisible() && !bottom->isCollapsed())
			lastBottomHeight = bottom->height();
	});
}

void MainWindow::buildDefaultPanels() {
	if (!d->activity) {
		return;
	}

	d->activity->addButton(QStringLiteral("settings"), QStringLiteral("⚙"), MainWindow::tr("Настройки"),
		ActivityBar::Position::Bottom, /*checkable*/ true);
}

// ------------------------------------------------------------------ actions
void MainWindow::Private::newDocument() {
	auto* registry = controller ? controller->widgets() : nullptr;
	if (!registry || registry->types().isEmpty()) {
		// Реестр недоступен (окно создано без контроллера): сохраняем
		// прежнее поведение — пустой текстовый документ.
		auto* doc = new TextDocument;
		doc->setTitle(MainWindow::tr("Документ %1").arg(untitledCounter++));
		q->addDocument(doc);
		return;
	}

	NewDocumentDialog dialog(registry, context, q);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}

	// Для singletone имя документа совпадает с xtype: повторный выбор
	// того же типа активирует уже открытый документ.
	createDocumentOfType(dialog.documentType(), dialog.documentName(),
		dialog.documentTitle(), dialog.documentGlyph(), dialog.variables());
}

QWidget* MainWindow::Private::createDocumentOfType(const QString& type, const QString& requestedId,
	const QString& title, const QString& glyph, const QVariantMap& variables) {
	auto* registry = controller ? controller->widgets() : nullptr;
	if (!registry) {
		q->log(MainWindow::tr("Реестр виджетов недоступен"));
		return nullptr;
	}

	const auto info = registry->typeInfo(type);
	if (!info) {
		q->log(MainWindow::tr("Тип документа не зарегистрирован: %1").arg(type));
		return nullptr;
	}

	// Роль документа определяет контекст и правила (см. DocumentRule).
	// Оформление по умолчанию (глиф, цвет вкладки) задаёт тема, поэтому
	// в регистрации типа остаются только идентичность и подпись.
	WidgetBuildHandler handler;
	handler.className = info->className;
	handler.xtype = info->xtype;
	handler.name = requestedId.isEmpty() ? info->xtype : requestedId;
	handler.title = title.isEmpty() ? info->title : title;
	handler.singletone = info->singletone;
	handler.rule = info->rule;
	handler.parent = q;
	handler.context = context;

	const auto id = registry->documentId(info->xtype, handler.name);
	auto* document = q->createWidget(id, WidgetKind::Document, handler.title, glyph,
		[registry, type = info->xtype, handler]() { return registry->build(type, handler); },
		QString(), info->xtype, context, variables);
	if (!document) {
		q->log(MainWindow::tr("Не удалось создать документ: %1").arg(info->xtype));
	}

	return document;
}

void MainWindow::Private::openSystemOptions() {
	// Документ системных настроек создаётся как обычный документ, но с ролью
	// DocumentRule::SystemOptions: его контекст — `.options` в режиме Mutable.
	auto* document = createDocumentOfType(SystemOptionsDocument::xtype(),
		SystemOptionsDocument::xtype(), QString(), QString());
	if (!document) {
		q->log(MainWindow::tr("Системные настройки недоступны: документ не зарегистрирован"));
	}
}

void MainWindow::Private::openFile(const QString& path) {
	for (auto* doc : area->documents()) {
		auto* td = qobject_cast<TextDocument*>(doc);
		if (td && td->filePath() == path) {
			area->setCurrentDocument(td);
			return;
		}
	}
	auto* doc = new TextDocument;
	if (!doc->loadFile(path)) {
		q->log(MainWindow::tr("Не удалось открыть: %1").arg(path));
		delete doc;
		return;
	}
	q->addDocument(doc);
}

void MainWindow::Private::toggleBottom() {
	if (!bottom) {
		return;
	}

	if (bottom->isVisible()) {
		bottom->hide();
	}
	else {
		bottom->show();
		bottom->setCollapsed(false);
	}
}

void MainWindow::Private::updateWindowTitle(DocumentWidget* doc) {
	const QString app = QStringLiteral("StudioShell");
	q->setWindowTitle(doc ? doc->title() + QStringLiteral(" — ") + app : app);
	statusDoc->setText(doc ? doc->title() : QString());
}

void MainWindow::Private::forgetDocument(DocumentWidget* doc) {
	for (auto it = entries.begin(); it != entries.end();) {
		if (it->documentIdentity == doc) {
			if (activeDocument == it.key()) {
				deactivateDocument();
			}
			it = entries.erase(it);
		}
		else
			it++;
	}
}

void MainWindow::Private::track(const QString& id, WidgetKind kind, QWidget* widget, const QString& title,
	const QString& type, DocumentWidget* document) {
	const auto serial = ++nextSerial;
	entries.insert(id, Entry{kind, widget, document, document, serial, title, type});
	QObject::connect(widget, &QObject::destroyed, q, [this, id, serial] {
		auto it = entries.find(id);
		if (it == entries.end() || it->serial != serial)
			return;
		const auto kind = it->kind;
		QPointer<DocumentWidget> doc = it->document;
		if (kind == WidgetKind::BottomPanel)
			q->unregisterBottomPanel(id);
		else if (kind == WidgetKind::SidePanel)
			q->unregisterSidePanel(id);
		else {
			entries.erase(it);
			if (doc)
				area->closeDocument(doc);
		}
	});
}

MainWindow::Options MainWindow::Options::fromBuildOptions(const QMap<QString, QVariant>& values) {
	Options options;
	options.bottomPanel = !values.value(QStringLiteral("bottombar.delete"), false).toBool();
	options.sidePanel = !values.value(QStringLiteral("sidebar.delete"), false).toBool();
	return options;
}

MainWindow::MainWindow(std::shared_ptr<CommandContext> context, QWidget* parent, bool buildDefaultPanels)
	: MainWindow(context, parent, buildDefaultPanels, Options()) {
}

MainWindow::MainWindow(std::shared_ptr<CommandContext> context, QWidget* parent, bool buildDefaultPanels,
	const Options& options)
	: QMainWindow(parent), d(std::make_unique<Private>(this)) {
	d->context = context;
	d->options = options;
	d->buildUi();
	d->buildMenus();
	d->wireSignals();
	if (context && context->commandController()) {
		d->controller = context->commandController();
		d->controller->setMainWindow(this);
	}

	log(tr("Приложение запущено"));
}

MainWindow::~MainWindow() {
	if (d->controller && d->controller->mainWindow() == this)
		d->controller->setMainWindow(nullptr);
	for (const auto& entry : d->entries) {
		if (entry.widget)
			entry.widget->disconnect(this);
	}
	d->area->disconnect(this);
	d->tabs->disconnect(this);
	if (d->side)
		d->side->disconnect(this);
	if (d->activity)
		d->activity->disconnect(this);
	if (d->bottom)
		d->bottom->disconnect(this);
}

QWidget* MainWindow::createWidget(const QString& id, WidgetKind kind, const QString& title,
	const QString& glyph, const WidgetFactory& factory, const QString& tooltip,
	const QString& type, const std::shared_ptr<CommandContext>& creatorContext,
	const QVariantMap& variables) {
	if (QThread::currentThread() != thread() || id.isEmpty() ||
		(kind != WidgetKind::Document && kind != WidgetKind::BottomPanel && kind != WidgetKind::SidePanel))
		return nullptr;
	if (!isZoneEnabled(kind)) {
		qWarning() << "MainWindow:"
			<< (kind == WidgetKind::BottomPanel ? "bottombar" : "sidebar")
			<< "is deleted by application description, widget rejected:" << id;
		return nullptr;
	}

	// Имя singletone-документа всегда равно его xtype: повторное создание
	// с другим id активирует уже открытый документ.
	QString documentId = id;
	DocumentRule rule = DocumentRule::Document;
	QString widgetXtype = type;
	QString widgetClass = type;
	if (d->controller && d->controller->widgets()) {
		auto* registry = d->controller->widgets();
		documentId = registry->documentId(type, id);

		// Роль документа (DocumentRule) описывает, в каком контексте он живёт
		// и какие правила применяет его interceptor. Тип и класс нужны панели
		// вкладок: по ним тема задаёт цвет вкладки.
		if (const auto info = registry->typeInfo(type)) {
			rule = info->rule;
			widgetXtype = info->xtype;
			widgetClass = info->className;
		}
	}

	if (auto* existing = widget(documentId)) {
		// Документ уже создан: другой документ его вызывает.
		const auto it = d->entries.constFind(documentId);
		if (it != d->entries.cend() && it->document && it->document->context()) {
			// Начальные переменные применяются и к существующему документу:
			// повторный вызов с path=... обновляет его состояние.
			for (auto variable = variables.cbegin(); variable != variables.cend(); variable++) {
				it->document->context()->setVariable(variable.key(), variable.value());
			}
		}
		if (it != d->entries.cend() && creatorContext) {
			creatorContext->notifyDocumentInvoked(documentId);
			if (it->document && it->document->context()) {
				it->document->context()->notifyDocumentInvoked(documentId);
			}
		}
		showWidget(documentId);
		return existing;
	}
	if (!factory || d->creating.contains(documentId))
		return nullptr;
	d->creating.insert(documentId);
	auto guard = qScopeGuard([this, documentId] { d->creating.remove(documentId); });
	QWidget* content = nullptr;
	try {
		content = factory();
	}
	catch (const std::exception& e) {
		qWarning() << "MainWindow: widget factory failed for" << documentId << e.what();
		return nullptr;
	}
	if (!content)
		return nullptr;
	for (const auto& entry : d->entries) {
		if (entry.widget == content || entry.document == content)
			return nullptr;
	}

	// Любой виджет любой зоны — документ: получает контекст и участвует
	// в общем дереве документов.
	auto* document = qobject_cast<DocumentWidget*>(content);
	if (!document) {
		document = new DocumentWidgetStub(content);
	}

	const auto creator = creatorContext ? creatorContext : d->context;
	std::shared_ptr<CommandContext> documentContext = document->context();
	if (!documentContext && creator) {
		documentContext = creator->createDocumentContext(documentId, rule);
	}
	if (documentContext) {
		// Начальные переменные пишутся до setContext: документ читает
		// их в onContextAttached() и создаёт недостающие значения.
		for (auto it = variables.cbegin(); it != variables.cend(); it++) {
			documentContext->setVariable(it.key(), it.value());
		}
		if (document->context() != documentContext) {
			document->setContext(documentContext);
		}
	}

	// Тип документа хранится в контексте: он используется панелью вкладок
	// для группировки и подсказками.
	if (!widgetXtype.isEmpty()) {
		document->setXtype(widgetXtype);
	}

	// Класс документа — привязка цвета вкладки из темы.
	if (!widgetClass.isEmpty()) {
		document->setClassName(widgetClass);
	}

	if (!title.isEmpty()) {
		document->setTitle(title);
	}
	if (!glyph.isEmpty()) {
		document->setGlyph(glyph);
	}
	document->setToolTip(tooltip);

	if (kind == WidgetKind::Document) {
		d->track(documentId, kind, document, title, type, document);
		addDocument(document);
	}
	else if (kind == WidgetKind::BottomPanel) {
		registerBottomPanel(documentId, glyph, title, document, tooltip, type);
		showBottomPanel(documentId);
	}
	else if (kind == WidgetKind::SidePanel) {
		registerSidePanel(documentId, glyph, title, document, tooltip, type);
		showSidePanel(documentId);
	}

	// Наружу отдаём документ: у всех зон это DocumentWidget, связанный контекстом.
	return document;
}

QWidget* MainWindow::widget(const QString& id) const {
	return d->entries.value(id).widget.data();
}

DocumentWidget* MainWindow::document(const QString& id) const {
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend()) {
		return nullptr;
	}

	return it->document;
}

std::shared_ptr<CommandContext> MainWindow::documentContext(const QString& id) const {
	auto* doc = document(id);
	return doc ? doc->context() : nullptr;
}

QString MainWindow::activeDocumentId() const {
	return d->activeDocument;
}

QString MainWindow::Private::idOfDocument(DocumentWidget* doc) const {
	if (!doc) {
		return { };
	}

	for (auto it = entries.cbegin(); it != entries.cend(); it++) {
		if (it->document.data() == doc) {
			return it.key();
		}
	}

	return { };
}

void MainWindow::Private::activateDocument(const QString& id) {
	if (id.isEmpty() || id == activeDocument) {
		return;
	}

	const auto it = entries.constFind(id);
	if (it == entries.cend() || !it->document) {
		return;
	}

	deactivateDocument();

	activeDocument = id;
	activeDocumentWidget = it->document;
	if (auto context = it->document->context()) {
		context->notifyDocumentActivated(id);
	}
}

void MainWindow::Private::deactivateDocument() {
	if (activeDocument.isEmpty()) {
		return;
	}

	QPointer<DocumentWidget> previous = activeDocumentWidget;
	activeDocument.clear();
	activeDocumentWidget = nullptr;

	if (previous) {
		if (auto context = previous->context()) {
			context->notifyDocumentDeactivated();
		}
	}
}

std::optional<MainWindow::WidgetKind> MainWindow::widgetKind(const QString& id) const {
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend() || !it->widget)
		return std::nullopt;
	return it->kind;
}

QString MainWindow::widgetTitle(const QString& id) const {
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend() || !it->widget)
		return {};
	return it->document ? it->document->title() : it->title;
}

QString MainWindow::widgetType(const QString& id) const {
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend() || !it->widget)
		return {};
	return it->type;
}

QStringList MainWindow::widgetIds() const {
	QStringList ids;
	for (auto it = d->entries.cbegin(); it != d->entries.cend(); it++)
		if (it->widget)
			ids.append(it.key());
	ids.sort();
	return ids;
}

QStringList MainWindow::widgetIds(WidgetKind kind) const {
	QStringList ids;
	for (auto it = d->entries.cbegin(); it != d->entries.cend(); it++)
		if (it->widget && it->kind == kind)
			ids.append(it.key());
	ids.sort();
	return ids;
}

bool MainWindow::isZoneEnabled(WidgetKind kind) const {
	switch (kind) {
	case WidgetKind::BottomPanel:
		return d->bottom != nullptr;
	case WidgetKind::SidePanel:
		return d->side != nullptr;
	case WidgetKind::Document:
		break;
	}
	return true;
}

void MainWindow::setDocumentTabsVisible(bool visible) {
	if (d->documentTabsVisible == visible) {
		return;
	}
	d->documentTabsVisible = visible;

	auto sizes = d->mainSplitter->sizes();   // [tabs, center, side]
	if (visible) {
		d->tabs->show();
		if (sizes.size() < 2) {
			return;
		}
		sizes[0] = d->lastTabsWidth;
		sizes[1] = qMax(200, sizes[1] - d->lastTabsWidth);
		d->mainSplitter->setSizes(sizes);
	}
	else {
		if (sizes.size() >= 2 && sizes[0] > 0) {
			d->lastTabsWidth = sizes[0];
		}
		d->tabs->hide();
	}
}

bool MainWindow::isDocumentTabsVisible() const {
	return d->documentTabsVisible;
}

bool MainWindow::showWidget(const QString& id) {
	if (QThread::currentThread() != thread())
		return false;
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend() || !it->widget)
		return false;
	if (it->kind == WidgetKind::Document) {
		if (!it->document)
			return false;
		d->area->setCurrentDocument(it->document);
	}
	else if (it->kind == WidgetKind::BottomPanel)
		showBottomPanel(id);
	else
		showSidePanel(id);
	return true;
}

bool MainWindow::closeWidget(const QString& id) {
	if (QThread::currentThread() != thread())
		return false;
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend() || !it->widget)
		return false;
	const auto kind = it->kind;
	QPointer<QWidget> content = it->widget;
	QPointer<DocumentWidget> doc = it->document;
	if (kind == WidgetKind::Document)
		return doc && d->area->closeDocument(doc);
	if (!content->close())
		return false;
	if (kind == WidgetKind::BottomPanel)
		unregisterBottomPanel(id);
	else
		unregisterSidePanel(id);
	return true;
}

DocumentArea* MainWindow::documentArea() const {
	return d->area;
}

void MainWindow::addDocument(DocumentWidget* doc) {
	d->area->addDocument(doc);
}

PropertiesPanel* MainWindow::Private::findPropertiesPanel() const {
	for (auto it = entries.cbegin(); it != entries.cend(); it++) {
		if (!it->widget) {
			continue;
		}

		if (auto* panel = qobject_cast<PropertiesPanel*>(it->widget.data())) {
			return panel;
		}

		if (auto* panel = it->widget->findChild<PropertiesPanel*>()) {
			return panel;
		}
	}

	return nullptr;
}

QString MainWindow::Private::propertiesPanelEntryId() const {
	auto* panel = findPropertiesPanel();
	if (!panel) {
		return { };
	}

	for (auto it = entries.cbegin(); it != entries.cend(); it++) {
		if (!it->widget) {
			continue;
		}

		if (it->widget.data() == panel || it->widget->findChild<PropertiesPanel*>() == panel) {
			return it.key();
		}
	}

	return { };
}

PropertiesPanel* MainWindow::propertiesPanel() const {
	return d->findPropertiesPanel();
}

bool MainWindow::showDocumentProperties(DocumentWidget* doc, const QString& groupId) {
	if (!doc || QThread::currentThread() != thread()) {
		return false;
	}

	auto* panel = propertiesPanel();
	if (!panel) {
		return false;
	}

	// Документ из области документов становится активным: панель свойств,
	// как и вкладки, следует за активным документом.
	for (auto it = d->entries.cbegin(); it != d->entries.cend(); it++) {
		if (it->kind == WidgetKind::Document && it->document == doc) {
			d->area->setCurrentDocument(doc);
			break;
		}
	}

	panel->setDocument(doc);
	if (!groupId.isEmpty()) {
		panel->revealGroup(groupId);
	}

	// Панель свойств регистрируется описанием приложения (обычно как боковая
	// панель): раскрываем её, чтобы группа была видна.
	const auto panelId = d->propertiesPanelEntryId();
	if (!panelId.isEmpty()) {
		showWidget(panelId);
	}

	return true;
}

void MainWindow::openFile(const QString& path) {
	d->openFile(path);
}

void MainWindow::registerSidePanel(const QString& id, const QString& glyph,
	const QString& title, QWidget* panel, const QString& tooltip, const QString& type) {
	if (id.isEmpty() || !panel)
		return;
	if (!d->side) {
		// Зона удалена описанием приложения (sidebar delete: true).
		qWarning() << "MainWindow: sidebar is deleted by application description, panel rejected:"
			<< id << type;
		return;
	}
	if (auto* existing = widget(id)) {
		if (existing != panel)
			panel->deleteLater();
		return;
	}
	d->track(id, WidgetKind::SidePanel, panel, title, type,
		qobject_cast<DocumentWidget*>(panel));
	d->side->addPanel(id, title, panel);
	d->activity->addButton(id, glyph, tooltip.isEmpty() ? title : tooltip);

	auto* act = new QAction(glyph + QLatin1Char(' ') + title, this);
	connect(act, &QAction::triggered, this, [this, id] { showSidePanel(id); });
	d->viewMenu->insertAction(d->sidePanelsSeparator, act);
	d->sideActions.insert(id, act);
}

void MainWindow::registerBottomPanel(const QString& id, const QString& glyph,
	const QString& title, QWidget* panel, const QString& tooltip, const QString& type) {
	if (id.isEmpty() || !panel)
		return;
	if (!d->bottom) {
		// Зона удалена описанием приложения (bottombar delete: true).
		qWarning() << "MainWindow: bottombar is deleted by application description, panel rejected:"
			<< id << type;
		return;
	}
	if (auto* existing = widget(id)) {
		if (existing != panel)
			panel->deleteLater();
		return;
	}
	d->track(id, WidgetKind::BottomPanel, panel, title, type,
		qobject_cast<DocumentWidget*>(panel));
	d->bottom->addPanel(id, glyph, title, panel);

	auto* act = new QAction(glyph + QLatin1Char(' ') + title, this);
	act->setStatusTip(tooltip);
	connect(act, &QAction::triggered, this, [this, id] { showBottomPanel(id); });
	d->viewMenu->insertAction(d->sidePanelsSeparator, act);
	d->bottomActions.insert(id, act);

	if (!tooltip.isEmpty()) {
		panel->setToolTip(tooltip);
	}
}

void MainWindow::unregisterBottomPanel(const QString& id) {
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend() || it->kind != WidgetKind::BottomPanel)
		return;
	if (!d->bottom)
		return;
	if (d->activeDocument == id)
		d->deactivateDocument();
	d->entries.remove(id);
	if (QAction* act = d->bottomActions.take(id)) {
		d->viewMenu->removeAction(act);
		act->deleteLater();
	}

	QWidget* page = d->bottom->panel(id);

	if (d->output && page == static_cast<QWidget*>(d->output))
		d->output = nullptr;

	if (d->bottom->removePanel(id))
		log(tr("Нижняя панель снята: %1").arg(id));
}

void MainWindow::unregisterSidePanel(const QString& id) {
	const auto it = d->entries.constFind(id);
	if (it == d->entries.cend() || it->kind != WidgetKind::SidePanel)
		return;
	if (!d->side || !d->activity)
		return;
	if (d->activeDocument == id)
		d->deactivateDocument();
	d->entries.remove(id);
	if (auto* act = d->sideActions.take(id)) {
		d->viewMenu->removeAction(act);
		act->deleteLater();
	}
	const bool current = d->side->currentPanel() == id;
	d->activity->removeButton(id);
	d->side->removePanel(id);
	if (current) {
		if (d->side->currentPanel().isEmpty())
			hideSidePanel();
		else
			d->activity->setActive(d->side->currentPanel());
	}
}

void MainWindow::showSidePanel(const QString& id) {
	if (!d->side) {
		qWarning() << "MainWindow: sidebar is deleted by application description, panel is not shown:"
			<< id;
		return;
	}

	d->side->setCurrentPanel(id);
	d->activateDocument(id);
	if (!d->side->isVisible()) {
		d->side->show();
		auto sizes = d->mainSplitter->sizes();   // [tabs, center, side]
		if (sizes.size() == 3) {
			sizes[1] = qMax(200, sizes[1] + sizes[2] - d->lastSideWidth);
			sizes[2] = d->lastSideWidth;
			d->mainSplitter->setSizes(sizes);
		}
	}

	if (d->activity)
		d->activity->setActive(id);
}

void MainWindow::hideSidePanel() {
	if (!d->side)
		return;
	if (d->side->isVisible())
		d->lastSideWidth = d->side->width();
	d->side->hide();
	if (d->activity)
		d->activity->setActive(QString());
}

void MainWindow::toggleSidePanel(const QString& id) {
	if (!d->side) {
		qWarning() << "MainWindow: sidebar is deleted by application description, panel is not toggled:"
			<< id;
		return;
	}
	if (d->side->isVisible() && d->side->currentPanel() == id)
		hideSidePanel();
	else
		showSidePanel(id);
}

void MainWindow::showBottomPanel(const QString& id) {
	if (!d->bottom) {
		qWarning() << "MainWindow: bottombar is deleted by application description, panel is not shown:"
			<< id;
		return;
	}
	d->bottom->show();
	d->bottom->setCurrentPanel(id);
	d->bottom->setCollapsed(false);
	d->activateDocument(id);
}

void MainWindow::log(const QString& message) {
	if (d->output)
		d->output->appendPlainText(QTime::currentTime().toString(QStringLiteral("HH:mm:ss  ")) + message);
}

void MainWindow::closeEvent(QCloseEvent* e) {
	if (d->area->closeAll())
		e->accept();
	else
		e->ignore();
}
