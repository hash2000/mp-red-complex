#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/doc_tab_panel/document_tabs_order.h"
#include "Libs/Engine/ui/glyph_tool_button.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/system_options/system_option.h"
#include "Libs/Resources/resources.h"

#include <QApplication>
#include <QEnterEvent>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

#include <functional>

namespace {

/// Полоса цвета вкладки. Рисуется вручную: цвета задаются пользователем,
/// поэтому их нельзя описать правилами темы.
class TabColorBar final : public QWidget {
public:
	explicit TabColorBar(QWidget* parent)
		: QWidget(parent) {
		setObjectName(QStringLiteral("DocumentTabColorBar"));
		setFixedWidth(3);
		setAttribute(Qt::WA_TransparentForMouseEvents);
		setVisible(false);
	}

	void setColor(const QColor& color) {
		if (_color == color) {
			return;
		}

		_color = color;
		setVisible(color.isValid());
		update();
	}

protected:
	void paintEvent(QPaintEvent*) override {
		if (!_color.isValid()) {
			return;
		}

		QPainter painter(this);
		painter.fillRect(rect().adjusted(0, 3, 0, -3), _color);
	}

private:
	QColor _color;
};

/// Одна «вкладка» слева: [цвет] [глиф] [заголовок........] [⚙] [✕]
/// Внутренняя деталь реализации, наружу не видна.
class DocumentTabItem final : public QFrame {
public:
	DocumentTabItem(DocumentWidget* doc, QWidget* parent)
		: QFrame(parent), _doc(doc) {
		setObjectName(QStringLiteral("DocumentTabItem"));
		setAttribute(Qt::WA_Hover);
		setMinimumHeight(28);

		auto* lay = new QHBoxLayout(this);
		lay->setContentsMargins(4, 2, 4, 2);
		lay->setSpacing(6);

		_color = new TabColorBar(this);
		_glyph = new QLabel(this);
		_glyph->setObjectName(QStringLiteral("DocumentTabGlyph"));
		_title = new QLabel(this);
		_title->setObjectName(QStringLiteral("DocTabTitle"));
		_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

		_props = new GlyphToolButton(QStringLiteral("⚙"),
			DocumentTabsPanel::tr("Свойства"), this, "DocTabButton");
		_close = new GlyphToolButton(QStringLiteral("✕"),
			DocumentTabsPanel::tr("Закрыть (средняя кнопка мыши)"), this, "DocTabButton");

		for (QToolButton* b : { _props, _close }) {
			QSizePolicy sp = b->sizePolicy();
			sp.setRetainSizeWhenHidden(true);   // без «прыжков» разметки
			b->setSizePolicy(sp);
		}

		lay->addWidget(_color);
		lay->addWidget(_glyph);
		lay->addWidget(_title, 1);
		lay->addWidget(_props);
		lay->addWidget(_close);

		connect(_props, &QToolButton::clicked, this, [this] { if (onProperties) onProperties(); });
		connect(_close, &QToolButton::clicked, this, [this] { if (onClose) onClose(); });

		updateFromDocument();
		updateButtons();
	}

	std::function<void()> onActivate;
	std::function<void()> onClose;
	std::function<void()> onProperties;
	std::function<void()> onDragStarted;
	std::function<void(const QPoint& globalPos)> onDragMoved;
	std::function<void()> onDragFinished;

	DocumentWidget* document() const {
		return _doc;
	}

	void setActive(bool active) {
		_active = active;
		setProperty("active", active);
		style()->unpolish(this);
		style()->polish(this);
		update();
		updateButtons();
	}

	/// Признак перетаскивания: элемент подсвечивается темой.
	void setDragging(bool dragging) {
		if (_dragging == dragging) {
			return;
		}

		_dragging = dragging;
		setProperty("dragging", dragging);
		style()->unpolish(this);
		style()->polish(this);
		update();
	}

	/// Действующий цвет вкладки: явный цвет документа важнее цвета темы.
	void setThemeColor(const QColor& color) {
		if (_themeColor == color) {
			return;
		}

		_themeColor = color;
		updateFromDocument();
	}

	void updateFromDocument() {
		_glyph->setText(_doc->glyph());
		const auto explicitColor = _doc->color();
		_color->setColor(explicitColor.isValid() ? explicitColor : _themeColor);
		_fullTitle = _doc->title() + (_doc->isModified() ? QStringLiteral(" ●") : QString());
		const auto tooltip = _doc->tabToolTip();
		setToolTip(tooltip.isEmpty() ? _doc->title() : tooltip);
		_title->setProperty("modified", _doc->isModified());
		style()->unpolish(this);
		style()->polish(this);
		update();
		updateElided();
	}

protected:
	void mousePressEvent(QMouseEvent* e) override {
		if (e->button() == Qt::LeftButton) {
			_pressed = true;
			_pressGlobal = e->globalPosition().toPoint();
			if (onActivate) onActivate();
		}
		else if (e->button() == Qt::MiddleButton) {
			if (onClose) onClose();
		}
		e->accept();
	}

	void mouseMoveEvent(QMouseEvent* e) override {
		if (!_pressed) {
			return;
		}

		const auto globalPos = e->globalPosition().toPoint();
		if (!_dragging && (_pressGlobal - globalPos).manhattanLength() >=
			QApplication::startDragDistance()) {
			_dragging = true;
			if (onDragStarted) onDragStarted();
		}

		if (_dragging && onDragMoved) {
			onDragMoved(globalPos);
		}

		e->accept();
	}

	void mouseReleaseEvent(QMouseEvent* e) override {
		_pressed = false;
		if (!_dragging) {
			return;
		}

		_dragging = false;
		setDragging(false);
		if (onDragFinished) onDragFinished();
		e->accept();
	}

	void enterEvent(QEnterEvent* e) override {
		_hover = true;  updateButtons(); QFrame::enterEvent(e);
	}
	void leaveEvent(QEvent* e)      override {
		_hover = false; updateButtons(); QFrame::leaveEvent(e);
	}
	void resizeEvent(QResizeEvent* e) override {
		QFrame::resizeEvent(e); updateElided();
	}

private:
	void updateButtons() {
		const bool show = _active || _hover;   // как в VS: кнопки у активной/под мышью
		_props->setVisible(show);
		_close->setVisible(show);
	}
	void updateElided() {
		_title->setText(_title->fontMetrics().elidedText(
			_fullTitle, Qt::ElideRight, qMax(0, _title->width())));
	}

	DocumentWidget* _doc;
	TabColorBar* _color;
	QLabel* _glyph;
	QLabel* _title;
	QToolButton* _props;
	QToolButton* _close;
	QString _fullTitle;
	/// Цвет вкладки из темы (привязка к классу документа/xtype).
	QColor _themeColor;
	bool _active = false;
	bool _hover = false;
	bool _pressed = false;
	bool _dragging = false;
	QPoint _pressGlobal;
};

} // namespace

struct DocumentTabsPanel::Private {
	explicit Private(DocumentTabsPanel* qq) : q(qq) {}

	DocumentTabsPanel* q;
	/// Контекст системных настроек (обычно глобальный).
	std::shared_ptr<CommandContext> settings;
	/// Реестр правил сортировки/группировки вкладок.
	DocumentTabsOrderRegistry orders;
	/// Активное правило.
	QString orderId;
	/// Цвета вкладок из темы.
	DocumentTabTheme theme;

	QScrollArea* scroll = nullptr;
	QWidget* list = nullptr;
	QVBoxLayout* listLayout = nullptr;
	QLabel* countLabel = nullptr;
	QHash<DocumentWidget*, DocumentTabItem*> items;
	/// Порядок ручной сортировки: перетаскивание меняет местами соседей.
	QList<DocumentWidget*> manualOrder;
	/// Заголовки групп в текущей разметке.
	QList<QLabel*> headers;
	QStringList groupTitles;
	QPointer<DocumentWidget> active;

	// Перетаскивание вкладки.
	DocumentTabItem* dragItem = nullptr;
	bool dragActive = false;
	int dropIndex = -1;

	void buildUi();
	void connectSettings();
	void applySettings();
	void loadTheme();

	/// Вкладки в порядке ручной сортировки с действующим цветом темы.
	QList<DocumentTabView> views() const;
	QList<DocumentTabGroup> arrangement();
	void rebuild();
	void updateCount();

	QList<DocumentTabItem*> itemsInLayoutOrder() const;
	int insertionIndexAt(int globalY) const;
	void clearDropMarker();
	void updateDropMarker(int index);

	void beginDrag(DocumentTabItem* item);
	void updateDrag(const QPoint& globalPos);
	void finishDrag(bool commit);
	void cancelDrag();
};

void DocumentTabsPanel::Private::buildUi() {
	q->setObjectName(QStringLiteral("DocumentTabsPanel"));
	q->setAttribute(Qt::WA_StyledBackground);
	q->setMinimumWidth(160);

	auto* root = new QVBoxLayout(q);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	// Заголовок
	auto* header = new QWidget(q);
	header->setObjectName(QStringLiteral("PanelHeader"));
	header->setAttribute(Qt::WA_StyledBackground);
	auto* hl = new QHBoxLayout(header);
	hl->setContentsMargins(10, 4, 4, 4);
	auto* title = new QLabel(DocumentTabsPanel::tr("ДОКУМЕНТЫ"), header);
	title->setObjectName(QStringLiteral("PanelTitle"));
	countLabel = new QLabel(header);
	countLabel->setObjectName(QStringLiteral("PanelCounter"));
	auto* newBtn = new GlyphToolButton(QStringLiteral("＋"),
		DocumentTabsPanel::tr("Новый документ (Ctrl+N)"), header);
	QObject::connect(newBtn, &QToolButton::clicked, q, &DocumentTabsPanel::newDocumentRequested);
	hl->addWidget(title);
	hl->addWidget(countLabel);
	hl->addStretch();
	hl->addWidget(newBtn);

	// Список вкладок
	scroll = new QScrollArea(q);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	list = new QWidget;
	list->setObjectName(QStringLiteral("DocumentTabsList"));
	list->setAttribute(Qt::WA_StyledBackground);
	listLayout = new QVBoxLayout(list);
	listLayout->setContentsMargins(4, 4, 4, 4);
	listLayout->setSpacing(2);
	listLayout->addStretch(1);
	scroll->setWidget(list);

	root->addWidget(header);
	root->addWidget(scroll, 1);

	updateCount();
}

void DocumentTabsPanel::Private::connectSettings() {
	if (!settings) {
		return;
	}

	QObject::connect(settings.get(), &CommandContext::variableChanged, q,
		[this](const QString&, const QString& name, const QVariant&, const QVariant&) {
			if (name == QLatin1String(SystemOptions::kDocumentsTabsOrder)) {
				applySettings();
			}
		});
}

void DocumentTabsPanel::Private::applySettings() {
	QString id = orders.defaultId();
	if (settings) {
		id = settings->variable(QString::fromLatin1(SystemOptions::kDocumentsTabsOrder), id).toString();
	}

	if (!q->setOrderStrategy(id)) {
		qWarning() << "DocumentTabsPanel: unknown tabs order strategy" << id
			<< "- using" << orders.defaultId();
		q->setOrderStrategy(orders.defaultId());
	}
}

void DocumentTabsPanel::Private::loadTheme() {
	// Тема применяется в BootstrapManager до создания оболочки, поэтому
	// каталог темы уже известен через ресурсы.
	const auto context = settings;
	auto* controller = context ? context->commandController() : nullptr;
	auto* resources = controller ? controller->resources() : nullptr;
	if (!resources) {
		return;
	}

	const auto themeDir = resources->Directories.get(QStringLiteral("path.theme"));
	if (!themeDir) {
		return;
	}

	theme = DocumentTabTheme::loadFromThemeDirectory(themeDir->absolutePath());
	if (!theme.isEmpty()) {
		qDebug() << "DocumentTabsPanel: tab theme loaded from"
			<< themeDir->absolutePath()
			<< "colors:" << theme.colors().size()
			<< "glyphs:" << theme.glyphs().size();
	}
}

QList<DocumentTabView> DocumentTabsPanel::Private::views() const {
	QList<DocumentTabView> result;
	result.reserve(manualOrder.size());
	for (auto* doc : manualOrder) {
		if (!items.contains(doc)) {
			continue;
		}

		DocumentTabView view;
		view.document = doc;
		// Привязка темы к классу документа или xtype: цвет документа важнее.
		view.themeColor = theme.colorFor(doc->className(), doc->xtype());
		result << view;
	}

	return result;
}

QList<DocumentTabGroup> DocumentTabsPanel::Private::arrangement() {
	const auto tabs = views();

	auto* strategy = orders.strategy(orderId);
	if (!strategy) {
		strategy = orders.strategy(orders.defaultId());
	}

	if (!strategy) {
		DocumentTabGroup group;
		group.tabs = tabs;
		return { group };
	}

	return strategy->arrange(tabs);
}

void DocumentTabsPanel::Private::updateCount() {
	countLabel->setText(items.isEmpty() ? QString() : QString::number(items.size()));
}

void DocumentTabsPanel::Private::rebuild() {
	// Тема задаёт стандартный глиф вкладки. Документ хранит его как запасной:
	// явный глиф документа важнее (см. DocumentWidget::glyph()).
	for (auto* doc : manualOrder) {
		if (!items.contains(doc)) {
			continue;
		}

		doc->setThemeGlyph(theme.glyphFor(doc->className(), doc->xtype()));
	}

	// Элементы вкладок переиспользуются, заголовки групп создаются заново.
	while (auto* layoutItem = listLayout->takeAt(0)) {
		delete layoutItem;
	}
	for (auto* item : items) {
		item->hide();
	}
	for (auto* header : headers) {
		delete header;
	}
	headers.clear();
	groupTitles.clear();

	int index = 0;
	for (const auto& group : arrangement()) {
		if (!group.title.isEmpty()) {
			auto* header = new QLabel(group.title, list);
			header->setObjectName(QStringLiteral("DocumentTabsGroup"));
			header->setProperty("groupColor",
				group.color.isValid() ? group.color.name() : QString());
			listLayout->insertWidget(index++, header);
			headers << header;
			groupTitles << group.title;
		}

		for (const auto& tab : group.tabs) {
			auto* item = items.value(tab.document);
			if (!item) {
				continue;
			}

			// Цвет из темы хранится в элементе: явный цвет документа важнее.
			item->setThemeColor(tab.themeColor);
			listLayout->insertWidget(index++, item);
			item->show();
		}
	}

	listLayout->addStretch(1);
	updateCount();

	// Разметка перестроена заново: маркер вставки нужно показать снова.
	if (dragActive) {
		updateDropMarker(dropIndex);
	}
}

QList<DocumentTabItem*> DocumentTabsPanel::Private::itemsInLayoutOrder() const {
	QList<DocumentTabItem*> result;
	for (int i = 0; i < listLayout->count(); i++) {
		// dynamic_cast, а не qobject_cast: элемент — внутренний класс без Q_OBJECT.
		auto* item = dynamic_cast<DocumentTabItem*>(listLayout->itemAt(i)->widget());
		if (item) {
			result << item;
		}
	}

	return result;
}

int DocumentTabsPanel::Private::insertionIndexAt(int globalY) const {
	int index = 0;
	for (auto* item : itemsInLayoutOrder()) {
		const int center = item->mapToGlobal(QPoint(0, item->height() / 2)).y();
		if (globalY < center) {
			return index;
		}

		index++;
	}

	return index;
}

void DocumentTabsPanel::Private::clearDropMarker() {
	for (auto* item : items) {
		if (!item->property("drop").isValid()) {
			continue;
		}

		item->setProperty("drop", QVariant());
		item->style()->unpolish(item);
		item->style()->polish(item);
		item->update();
	}
}

void DocumentTabsPanel::Private::updateDropMarker(int index) {
	clearDropMarker();

	const auto visibleItems = itemsInLayoutOrder();
	if (visibleItems.isEmpty()) {
		return;
	}

	DocumentTabItem* target = nullptr;
	QString marker;
	if (index < 0 || index >= visibleItems.size()) {
		target = visibleItems.last();
		marker = QStringLiteral("after");
	}
	else {
		target = visibleItems.at(index);
		marker = QStringLiteral("before");
	}

	target->setProperty("drop", marker);
	target->style()->unpolish(target);
	target->style()->polish(target);
	target->update();
}

void DocumentTabsPanel::Private::beginDrag(DocumentTabItem* item) {
	const auto* strategy = orders.strategy(orderId);
	if (!item || !strategy || !strategy->supportsManualMove()) {
		return;
	}

	cancelDrag();
	dragItem = item;
	dragActive = true;
	item->setDragging(true);
}

void DocumentTabsPanel::Private::updateDrag(const QPoint& globalPos) {
	if (!dragActive || !dragItem) {
		return;
	}

	dropIndex = insertionIndexAt(globalPos.y());
	updateDropMarker(dropIndex);
}

void DocumentTabsPanel::Private::finishDrag(bool commit) {
	if (!dragActive || !dragItem) {
		cancelDrag();
		return;
	}

	auto* item = dragItem;
	const int target = dropIndex;
	auto* doc = item->document();
	cancelDrag();

	if (!commit || target < 0 || !doc) {
		return;
	}

	const int current = manualOrder.indexOf(doc);
	if (current < 0) {
		return;
	}

	// Индекс вставки считается по разметке: после изъятия документа
	// все позиции ниже сдвигаются на одну.
	int index = target;
	if (index > current) {
		index--;
	}

	q->moveDocument(doc, index);
}

void DocumentTabsPanel::Private::cancelDrag() {
	if (dragItem) {
		dragItem->setDragging(false);
	}
	dragItem = nullptr;
	dragActive = false;
	dropIndex = -1;
	clearDropMarker();
}

DocumentTabsPanel::DocumentTabsPanel(QWidget* parent)
	: DocumentTabsPanel(std::shared_ptr<CommandContext>(), parent) {
}

DocumentTabsPanel::DocumentTabsPanel(std::shared_ptr<CommandContext> settingsContext, QWidget* parent)
	: QWidget(parent), d(std::make_unique<Private>(this)) {
	d->settings = settingsContext;
	d->orderId = d->orders.defaultId();
	d->buildUi();
	d->connectSettings();
	d->loadTheme();
	d->applySettings();
}

DocumentTabsPanel::~DocumentTabsPanel() = default;

void DocumentTabsPanel::addDocument(DocumentWidget* doc) {
	if (!doc || d->items.contains(doc)) {
		return;
	}

	auto* item = new DocumentTabItem(doc, d->list);
	item->onActivate = [this, doc] { emit activateRequested(doc); };
	item->onClose = [this, doc] { emit closeRequested(doc); };
	item->onProperties = [this, doc] { emit propertiesRequested(doc); };
	item->onDragStarted = [this, item] { d->beginDrag(item); };
	item->onDragMoved = [this](const QPoint& globalPos) { d->updateDrag(globalPos); };
	item->onDragFinished = [this] { d->finishDrag(true); };

	auto refresh = [item] { item->updateFromDocument(); };
	connect(doc, &DocumentWidget::titleChanged, item, refresh);
	connect(doc, &DocumentWidget::glyphChanged, item, refresh);
	connect(doc, &DocumentWidget::modifiedChanged, item, refresh);
	connect(doc, &DocumentWidget::colorChanged, item, refresh);
	connect(doc, &DocumentWidget::tabToolTipChanged, item, refresh);

	// Цвет вкладки и тип документа влияют на группировку.
	connect(doc, &DocumentWidget::colorChanged, this, [this] { d->rebuild(); });
	connect(doc, &DocumentWidget::xtypeChanged, this, [this] { d->rebuild(); });

	d->items.insert(doc, item);
	d->manualOrder.append(doc);
	d->rebuild();
}

void DocumentTabsPanel::removeDocument(DocumentWidget* doc) {
	if (auto* item = d->items.take(doc)) {
		if (d->dragItem == item) {
			d->cancelDrag();
		}

		item->hide();
		item->deleteLater();   // мы можем находиться внутри клика по кнопке этого item
	}
	d->manualOrder.removeAll(doc);
	if (d->active == doc) {
		d->active = nullptr;
	}
	d->rebuild();
}

void DocumentTabsPanel::setActiveDocument(DocumentWidget* doc) {
	if (auto* prev = d->items.value(d->active.data())) {
		prev->setActive(false);
	}
	d->active = doc;
	if (auto* cur = d->items.value(doc)) {
		cur->setActive(true);
		d->scroll->ensureWidgetVisible(cur);
	}
}

QList<DocumentWidget*> DocumentTabsPanel::documents() const {
	return d->manualOrder;
}

bool DocumentTabsPanel::moveDocument(DocumentWidget* doc, int index) {
	const int current = d->manualOrder.indexOf(doc);
	if (current < 0) {
		return false;
	}

	const int target = qBound(0, index, int(d->manualOrder.size()) - 1);
	if (target == current) {
		return true;
	}

	d->manualOrder.move(current, target);
	d->rebuild();
	return true;
}

QString DocumentTabsPanel::orderStrategy() const {
	return d->orderId;
}

bool DocumentTabsPanel::setOrderStrategy(const QString& id) {
	if (!d->orders.strategy(id)) {
		return false;
	}

	if (d->orderId == id) {
		return true;
	}

	d->orderId = id;
	d->cancelDrag();
	d->rebuild();
	return true;
}

void DocumentTabsPanel::setDocumentTabTheme(const DocumentTabTheme& theme) {
	d->theme = theme;
	d->rebuild();
}

const DocumentTabTheme& DocumentTabsPanel::documentTabTheme() const {
	return d->theme;
}

QStringList DocumentTabsPanel::groupTitles() const {
	return d->groupTitles;
}
