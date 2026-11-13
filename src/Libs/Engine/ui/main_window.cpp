#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/activity_bar.h"
#include "Libs/Engine/ui/bottom_panel.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/side_host_panel.h"
#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Engine/ui/text_document.h"
#include "Libs/Engine/ui/command_console.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTime>
#include <QTreeView>

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

	DocumentArea* area = nullptr;
	DocumentTabsPanel* tabs = nullptr;
	SideHostPanel* side = nullptr;
	ActivityBar* activity = nullptr;
	BottomPanel* bottom = nullptr;
	PropertiesPanel* properties = nullptr;
	QSplitter* mainSplitter = nullptr;    // [tabs | center | side]
	QSplitter* centerSplitter = nullptr;  // [area / bottom]
	QPlainTextEdit* output = nullptr;
	QLabel* statusDoc = nullptr;
	QMenu* viewMenu = nullptr;
	QAction* sidePanelsSeparator = nullptr;
	QAction* pinAction = nullptr;

	QHash<QString, QAction*> bottomActions;

	std::shared_ptr<CommandContext> context;

	int untitledCounter = 0;
	int lastSideWidth = 280;
	int lastBottomHeight = 220;

	void buildUi();
	void buildMenus();
	void wireSignals();
	void buildDefaultPanels();

	void newDocument();
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

	// левая панель
	tabs = new DocumentTabsPanel;

	centerSplitter = new QSplitter(Qt::Vertical);
	centerSplitter->setHandleWidth(1);
	centerSplitter->setChildrenCollapsible(false);

	// центральная панель, область открытого документа
	area = new DocumentArea;

	// информационная панель снизу
	bottom = new BottomPanel;
	centerSplitter->addWidget(area);
	centerSplitter->addWidget(bottom);
	centerSplitter->setStretchFactor(0, 1);
	centerSplitter->setStretchFactor(1, 0);

	// область панелей справа
	side = new SideHostPanel;

	mainSplitter->addWidget(tabs);
	mainSplitter->addWidget(centerSplitter);
	mainSplitter->addWidget(side);
	mainSplitter->setStretchFactor(0, 0);
	mainSplitter->setStretchFactor(1, 1);
	mainSplitter->setStretchFactor(2, 0);
	mainSplitter->setSizes({ 220, 900, lastSideWidth });
	centerSplitter->setSizes({ 600, lastBottomHeight });

	// область кнопок панелей справа
	activity = new ActivityBar(central);

	h->addWidget(mainSplitter, 1);
	h->addWidget(activity);
	q->setCentralWidget(central);

	// Статус-бар
	q->statusBar()->addWidget(new QLabel(MainWindow::tr("Готово")));
	statusDoc = new QLabel;
	q->statusBar()->addPermanentWidget(statusDoc);

	// Панель свойств создаётся всегда: к ней привязан активный документ,
	// а регистрируется она либо по умолчанию, либо из yaml-описания.
	properties = new PropertiesPanel;

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
	viewMenu->addAction(MainWindow::tr("Нижняя панель"), QKeySequence(Qt::CTRL | Qt::Key_J),
		q, [this] { toggleBottom(); });
	viewMenu->addSeparator();
	pinAction = viewMenu->addAction(MainWindow::tr("📌 Закрепить панель кнопок"));
	pinAction->setCheckable(true);
	pinAction->setChecked(true);
	QObject::connect(pinAction, &QAction::toggled, side, [this](bool pinned) {
		activity->setAutoHide(!pinned);
		pinAction->setChecked(pinned);
	});
}

void MainWindow::Private::wireSignals() {
	// Левая панель вкладок -> область документов
	QObject::connect(tabs, &DocumentTabsPanel::activateRequested, area, &DocumentArea::setCurrentDocument);
	QObject::connect(tabs, &DocumentTabsPanel::closeRequested, area, &DocumentArea::closeDocument);
	QObject::connect(tabs, &DocumentTabsPanel::newDocumentRequested, q, [this] { newDocument(); });
	QObject::connect(tabs, &DocumentTabsPanel::propertiesRequested, q, [this](DocumentWidget* doc) {
		area->setCurrentDocument(doc);
		properties->setDocument(doc);
		q->showSidePanel(QStringLiteral("properties"));
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
		tabs->removeDocument(doc);
		if (properties->document() == doc)
			properties->setDocument(nullptr);
		q->log(MainWindow::tr("Закрыт документ: %1").arg(doc->title()));
	});
	QObject::connect(area, &DocumentArea::currentDocumentChanged, q, [this](DocumentWidget* doc) {
		tabs->setActiveDocument(doc);
		properties->setDocument(doc);   // как в VS: «Свойства» следуют за активным документом
		updateWindowTitle(doc);
	});

	// Правая сторона
	QObject::connect(activity, &ActivityBar::buttonClicked, q, [this](const QString& id) {
		if (id == QLatin1String("settings")) {
			q->log(MainWindow::tr("Настройки: ещё не реализовано"));
			return;
		}
		q->toggleSidePanel(id);
	});
	QObject::connect(side, &SideHostPanel::closeRequested, q, [this] { q->hideSidePanel(); });

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

	// Запоминаем размеры, выставленные пользователем
	QObject::connect(mainSplitter, &QSplitter::splitterMoved, q, [this] {
		if (side->isVisible())
			lastSideWidth = side->width();
	});
	QObject::connect(centerSplitter, &QSplitter::splitterMoved, q, [this] {
		if (bottom->isVisible() && !bottom->isCollapsed())
			lastBottomHeight = bottom->height();
	});
}

void MainWindow::Private::buildDefaultPanels() {
	// --- Правые панели (ваши виджеты подставляются сюда) ---
	auto* model = new QFileSystemModel(q);
	model->setRootPath(QDir::currentPath());
	auto* tree = new QTreeView;
	tree->setModel(model);
	tree->setRootIndex(model->index(QDir::currentPath()));
	tree->setHeaderHidden(true);
	for (int c = 1; c < model->columnCount(); c++)
		tree->hideColumn(c);
	QObject::connect(tree, &QTreeView::doubleClicked, q, [this, model](const QModelIndex& idx) {
		if (!model->isDir(idx))
			openFile(model->filePath(idx));
	});

	activity->addButton(QStringLiteral("settings"), QStringLiteral("⚙"), MainWindow::tr("Настройки"),
		ActivityBar::Position::Bottom, /*checkable*/ true);

	// --- Нижние панели ---
	output = new QPlainTextEdit;
	output->setReadOnly(true);

	auto* errors = new QTableWidget(0, 4);
	errors->setHorizontalHeaderLabels({ MainWindow::tr("Код"), MainWindow::tr("Описание"),
		MainWindow::tr("Файл"), MainWindow::tr("Строка") });
	errors->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	errors->verticalHeader()->hide();

	q->showSidePanel(QStringLiteral("explorer"));
	bottom->setCurrentPanel(QStringLiteral("console"));
}

// ------------------------------------------------------------------ actions
void MainWindow::Private::newDocument() {
	auto* doc = new TextDocument;
	doc->setTitle(MainWindow::tr("Документ %1").arg(++untitledCounter));
	q->addDocument(doc);
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

MainWindow::MainWindow(std::shared_ptr<CommandContext> context, QWidget* parent, bool buildDefaultPanels)
	: QMainWindow(parent), d(std::make_unique<Private>(this)) {
	d->context = context;
	d->buildUi();
	d->buildMenus();
	d->wireSignals();

	if (buildDefaultPanels) {
		d->buildDefaultPanels();
	}

	log(tr("Приложение запущено"));
}

MainWindow::~MainWindow() = default;

DocumentArea* MainWindow::documentArea() const {
	return d->area;
}

void MainWindow::addDocument(DocumentWidget* doc) {
	d->area->addDocument(doc);
}

PropertiesPanel* MainWindow::propertiesPanel() const {
	return d->properties;
}

void MainWindow::openFile(const QString& path) {
	d->openFile(path);
}

void MainWindow::registerSidePanel(const QString& id, const QString& glyph,
	const QString& title, QWidget* panel, const QString& tooltip) {
	d->side->addPanel(id, title, panel);
	d->activity->addButton(id, glyph, tooltip.isEmpty() ? title : tooltip);

	auto* act = new QAction(glyph + QLatin1Char(' ') + title, this);
	connect(act, &QAction::triggered, this, [this, id] { showSidePanel(id); });
	d->viewMenu->insertAction(d->sidePanelsSeparator, act);
}

void MainWindow::registerBottomPanel(const QString& id, const QString& glyph,
	const QString& title, QWidget* panel, const QString& tooltip) {
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

void MainWindow::showSidePanel(const QString& id) {
	d->side->setCurrentPanel(id);
	if (!d->side->isVisible()) {
		d->side->show();
		auto sizes = d->mainSplitter->sizes();   // [tabs, center, side]
		if (sizes.size() == 3) {
			sizes[1] = qMax(200, sizes[1] + sizes[2] - d->lastSideWidth);
			sizes[2] = d->lastSideWidth;
			d->mainSplitter->setSizes(sizes);
		}
	}

	d->activity->setActive(id);
}

void MainWindow::hideSidePanel() {
	if (d->side->isVisible())
		d->lastSideWidth = d->side->width();
	d->side->hide();
	d->activity->setActive(QString());
}

void MainWindow::toggleSidePanel(const QString& id) {
	if (d->side->isVisible() && d->side->currentPanel() == id)
		hideSidePanel();
	else
		showSidePanel(id);
}

void MainWindow::showBottomPanel(const QString& id) {
	d->bottom->show();
	d->bottom->setCurrentPanel(id);
	d->bottom->setCollapsed(false);
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
