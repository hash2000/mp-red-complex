#include "Content/SystemRouter/widgets/system_router_node_view_widget.h"
#include "Content/SystemRouter/models/system_router_command.h"
#include "Content/SystemRouter/models/system_router_mime.h"
#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Libs/BaseWidgets/properties/property_mime.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_glyph.h"
#include "Libs/Engine/ui/glyph_tool_button.h"

#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr const char* kCopyNameGlyph = "⧉";
constexpr const char* kCopyValueGlyph = "📋";
/// Предел текста для показа в редакторе значения: полное значение остаётся
/// в параметре и копируется кнопкой.
constexpr int kMaxDisplayTextLength = 4000;
constexpr int kMultilineEditorHeight = 120;
constexpr int kImagePreviewWidth = 320;
constexpr int kImagePreviewHeight = 240;

/// Служебные mime-типы маршрутизатора не определяют вид значения.
QStringList ignoredValueMimeTypes() {
	return {
		QString::fromLatin1(SystemRouterMime::kCommand),
		QString::fromLatin1(SystemRouterMime::kCommandParameter)
	};
}

bool hasMime(const QStringList& mimeTypes, const char* mimeType) {
	for (const auto& raw : mimeTypes) {
		if (raw.trimmed().compare(QLatin1String(mimeType), Qt::CaseInsensitive) == 0) {
			return true;
		}
	}

	return false;
}

QString truncateForDisplay(const QString& text) {
	if (text.size() <= kMaxDisplayTextLength) {
		return text;
	}

	return text.left(kMaxDisplayTextLength) + QStringLiteral("…");
}

/// Отображаемый текст значения: структуры и бинарные данные приводятся
/// к тексту так же, как их видит редактор свойств.
QString valueText(const QVariant& value, const QString& mimeType) {
	return truncateForDisplay(PropertyMime::normalizeValue(value, mimeType).toString());
}

QString valueToClipboardText(const QVariant& value) {
	const auto type = value.metaType().id();
	if (type == QMetaType::QVariantMap || type == QMetaType::QVariantList) {
		return QString::fromUtf8(
			QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact));
	}
	if (type == QMetaType::QByteArray) {
		return QString::fromUtf8(value.toByteArray());
	}
	if (type == QMetaType::QColor) {
		return value.value<QColor>().name(QColor::HexArgb);
	}

	return value.toString();
}

/// Красиво отформатированный JSON для многострочного просмотра.
QString formattedJson(const QVariant& value) {
	const auto type = value.metaType().id();
	QJsonDocument document;
	if (type == QMetaType::QVariantMap) {
		document = QJsonDocument::fromVariant(value);
	}
	else if (type == QMetaType::QVariantList) {
		document = QJsonDocument::fromVariant(value);
	}
	else {
		QJsonParseError error;
		document = QJsonDocument::fromJson(value.toString().toUtf8(), &error);
		if (error.error != QJsonParseError::NoError) {
			return valueText(value, QString::fromLatin1(PropertyMimeType::kJson));
		}
	}

	return QString::fromUtf8(document.toJson(QJsonDocument::Indented));
}

/// Изображение из значения параметра: путь к файлу, data-URL или base64.
QPixmap pixmapForValue(const QVariant& value) {
	QPixmap pixmap;
	if (value.metaType().id() == QMetaType::QByteArray) {
		pixmap.loadFromData(value.toByteArray());
		return pixmap;
	}

	const auto text = value.toString().trimmed();
	if (text.isEmpty()) {
		return pixmap;
	}

	if (text.startsWith(QLatin1String("data:image/"), Qt::CaseInsensitive)) {
		const int comma = text.indexOf(QLatin1Char(','));
		if (comma > 0) {
			pixmap.loadFromData(QByteArray::fromBase64(text.mid(comma + 1).toLatin1()));
		}
		return pixmap;
	}

	if (QFileInfo::exists(text)) {
		pixmap.load(text);
		return pixmap;
	}

	// Сырые base64-данные изображения.
	pixmap.loadFromData(QByteArray::fromBase64(text.toLatin1()));
	return pixmap;
}

QStringList sortedPropertyNames(const LazyTreeNodePtr& node) {
	QStringList names = node ? node->properties().keys() : QStringList();
	std::sort(names.begin(), names.end(), [](const QString& left, const QString& right) {
		return QString::localeAwareCompare(left, right) < 0;
	});
	return names;
}

} // namespace

class SystemRouterNodeViewWidget::Private {
public:
	explicit Private(SystemRouterNodeViewWidget* parent)
		: q(parent) {
	}

	SystemRouterNodeViewWidget* q;
	SystemRouterService* service = nullptr;
	SystemRouterCommandService* commandService = nullptr;
	QVariant nodeId;
	LazyTreeNodePtr node;
	QStringList names;

	QLabel* glyphLabel = nullptr;
	QLabel* nodeNameLabel = nullptr;
	QLabel* nodeIdLabel = nullptr;
	QLabel* empty = nullptr;
	QLabel* status = nullptr;
	QTimer* statusTimer = nullptr;
	QScrollArea* scroll = nullptr;
	QWidget* content = nullptr;
	QVBoxLayout* contentLayout = nullptr;

	void setupUi();
	void loadNode();
	void rebuild();
	void clearParameters();
	void updateHeader();
	void addParameter(const QString& name, const QStringList& mimeTypes);
	QWidget* createValueEditor(const QString& name, const QStringList& mimeTypes);
	QWidget* createCommandEditor(const QVariant& value);
	QWidget* createImageEditor(const QVariant& value);
	QWidget* createColorEditor(const QVariant& value);
	QWidget* createBooleanEditor(bool value);
	QWidget* createTextEditor(const QString& text);
	QWidget* createMultilineEditor(const QString& text);
	QString propertyTooltip(const QStringList& mimeTypes) const;
	QString propertyClipboardText(const QString& name, const QStringList& mimeTypes) const;
	void copyToClipboard(const QString& text, const QString& message);
};

void SystemRouterNodeViewWidget::Private::setupUi() {
	auto* layout = new QVBoxLayout(q);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->setSpacing(4);

	// Информация об узле: глиф и название (h3), затем идентификатор (h4).
	// Отступы, цвета и размеры шрифтов задаёт тема (system-router.qss).
	auto* header = new QWidget(q);
	header->setObjectName(QStringLiteral("SystemRouterNodeViewHeader"));
	auto* headerLayout = new QVBoxLayout(header);
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setSpacing(0);

	auto* titleRow = new QHBoxLayout();
	titleRow->setContentsMargins(0, 0, 0, 0);
	titleRow->setSpacing(6);

	glyphLabel = new QLabel(header);
	glyphLabel->setObjectName(QStringLiteral("SystemRouterNodeViewGlyph"));
	glyphLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	titleRow->addWidget(glyphLabel, 0, Qt::AlignVCenter);

	nodeNameLabel = new QLabel(header);
	nodeNameLabel->setObjectName(QStringLiteral("SystemRouterNodeViewNodeName"));
	nodeNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	nodeNameLabel->setWordWrap(true);
	titleRow->addWidget(nodeNameLabel, 1, Qt::AlignVCenter);
	headerLayout->addLayout(titleRow);

	nodeIdLabel = new QLabel(header);
	nodeIdLabel->setObjectName(QStringLiteral("SystemRouterNodeViewNodeId"));
	nodeIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	nodeIdLabel->setWordWrap(true);
	headerLayout->addWidget(nodeIdLabel);

	layout->addWidget(header);

	scroll = new QScrollArea(q);
	scroll->setObjectName(QStringLiteral("SystemRouterNodeViewScroll"));
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	content = new QWidget(scroll);
	content->setObjectName(QStringLiteral("SystemRouterNodeViewContent"));
	contentLayout = new QVBoxLayout(content);
	contentLayout->setContentsMargins(0, 0, 0, 0);
	contentLayout->setSpacing(6);

	scroll->setWidget(content);
	layout->addWidget(scroll, 1);

	empty = new QLabel(SystemRouterNodeViewWidget::tr("У узла нет параметров"), q);
	empty->setObjectName(QStringLiteral("SystemRouterNodeViewEmpty"));
	empty->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	empty->setWordWrap(true);
	layout->addWidget(empty, 1);

	status = new QLabel(q);
	status->setObjectName(QStringLiteral("SystemRouterNodeViewStatus"));
	status->setWordWrap(true);
	status->hide();
	layout->addWidget(status);

	// Сообщение о копировании — кратковременная подсказка, а не состояние.
	statusTimer = new QTimer(q);
	statusTimer->setSingleShot(true);
	statusTimer->setInterval(3000);
	QObject::connect(statusTimer, &QTimer::timeout, status, &QLabel::hide);
}

void SystemRouterNodeViewWidget::Private::loadNode() {
	node = nullptr;
	if (service && nodeId.isValid() && !nodeId.isNull()) {
		node = service->item(nodeId);
	}
}

void SystemRouterNodeViewWidget::Private::clearParameters() {
	while (QLayoutItem* item = contentLayout->takeAt(0)) {
		if (auto* widget = item->widget()) {
			widget->hide();
			widget->setParent(nullptr);
			delete widget;
		}
		delete item;
	}
}

void SystemRouterNodeViewWidget::Private::updateHeader() {
	if (node) {
		glyphLabel->setText(LazyTreeNodeGlyph::resolved(node));
		glyphLabel->setVisible(true);
		nodeNameLabel->setText(node->name());
		nodeIdLabel->setText(SystemRouterNodeViewWidget::tr("id: %1")
			.arg(node->id().toString()));
		nodeIdLabel->setVisible(true);
		return;
	}

	glyphLabel->clear();
	glyphLabel->setVisible(false);
	nodeNameLabel->setText(nodeId.isValid() && !nodeId.isNull()
		? SystemRouterNodeViewWidget::tr("Узел не найден")
		: SystemRouterNodeViewWidget::tr("Узел не выбран"));
	nodeIdLabel->setText(nodeId.isValid() && !nodeId.isNull()
		? SystemRouterNodeViewWidget::tr("id: %1").arg(nodeId.toString())
		: QString());
	nodeIdLabel->setVisible(nodeId.isValid() && !nodeId.isNull());
}

void SystemRouterNodeViewWidget::Private::rebuild() {
	clearParameters();
	names.clear();
	updateHeader();

	if (node) {
		const auto propertyNames = sortedPropertyNames(node);
		for (const auto& name : propertyNames) {
			const auto mimeTypes = node->propertyMimeTypes(name);
			// Глиф узла — оформление, а не параметр; скрытые параметры
			// в просмотр не попадают вовсе.
			if (LazyTreeNodeGlyph::isProperty(name) || PropertyMime::isHidden(mimeTypes)) {
				continue;
			}

			addParameter(name, mimeTypes);
			names.append(name);
		}
	}

	// Распорка забирает свободное место по вертикали: параметры остаются
	// компактными у верхнего края, а при большом количестве появляется
	// вертикальная прокрутка.
	contentLayout->addStretch(1);

	const bool hasParameters = !names.isEmpty();
	scroll->setVisible(hasParameters);
	empty->setVisible(!hasParameters);
	empty->setText(node
		? SystemRouterNodeViewWidget::tr("У узла нет параметров")
		: SystemRouterNodeViewWidget::tr("Параметры не загружены"));
}

void SystemRouterNodeViewWidget::Private::addParameter(const QString& name,
	const QStringList& mimeTypes) {
	// Параметр — блок: строка названия (h3) со значками копирования, под ней
	// виджет значения (h4). Оформление задаёт тема.
	auto* block = new QWidget(content);
	block->setObjectName(QStringLiteral("SystemRouterNodeViewParameter"));
	auto* blockLayout = new QVBoxLayout(block);
	blockLayout->setContentsMargins(0, 0, 0, 0);
	blockLayout->setSpacing(2);

	auto* titleRow = new QHBoxLayout();
	titleRow->setContentsMargins(0, 0, 0, 0);
	titleRow->setSpacing(2);

	auto* nameLabel = new QLabel(name, block);
	nameLabel->setObjectName(QStringLiteral("SystemRouterNodeViewParameterName"));
	nameLabel->setProperty("propertyName", name);
	nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	nameLabel->setToolTip(propertyTooltip(mimeTypes));
	titleRow->addWidget(nameLabel, 0, Qt::AlignVCenter);

	auto* copyName = new GlyphToolButton(QString::fromUtf8(kCopyNameGlyph),
		SystemRouterNodeViewWidget::tr("Копировать имя параметра"), block);
	copyName->setObjectName(QStringLiteral("SystemRouterNodeViewCopyName"));
	copyName->setProperty("propertyId", name);
	QObject::connect(copyName, &QToolButton::clicked, q, [this, name]() {
		copyToClipboard(name, SystemRouterNodeViewWidget::tr("Имя параметра скопировано: %1")
			.arg(name));
	});
	titleRow->addWidget(copyName, 0, Qt::AlignVCenter);

	const bool secret = PropertyMime::isSecret(mimeTypes);
	auto* copyValue = new GlyphToolButton(QString::fromUtf8(kCopyValueGlyph),
		SystemRouterNodeViewWidget::tr("Копировать значение параметра"), block);
	copyValue->setObjectName(QStringLiteral("SystemRouterNodeViewCopyValue"));
	copyValue->setProperty("propertyId", name);
	copyValue->setEnabled(!secret);
	if (secret) {
		copyValue->setToolTip(SystemRouterNodeViewWidget::tr(
			"Значение секретного параметра недоступно"));
	}
	else {
		QObject::connect(copyValue, &QToolButton::clicked, q, [this, name, mimeTypes]() {
			copyToClipboard(propertyClipboardText(name, mimeTypes),
				SystemRouterNodeViewWidget::tr("Значение параметра скопировано: %1").arg(name));
		});
	}
	titleRow->addWidget(copyValue, 0, Qt::AlignVCenter);
	titleRow->addStretch(1);
	blockLayout->addLayout(titleRow);

	auto* valueEditor = createValueEditor(name, mimeTypes);
	valueEditor->setProperty("propertyId", name);
	blockLayout->addWidget(valueEditor);

	contentLayout->addWidget(block);
}

QWidget* SystemRouterNodeViewWidget::Private::createValueEditor(const QString& name,
	const QStringList& mimeTypes) {
	const auto value = node ? node->properties().value(name) : QVariant();

	if (PropertyMime::isSecret(mimeTypes)) {
		auto* secret = new QLabel(SystemRouterNodeViewWidget::tr("•••••• (значение скрыто)"), content);
		secret->setObjectName(QStringLiteral("SystemRouterNodeViewSecret"));
		secret->setToolTip(SystemRouterNodeViewWidget::tr(
			"Параметр помечен как секретный: значение не показывается"));
		return secret;
	}

	if (hasMime(mimeTypes, SystemRouterMime::kCommand)) {
		return createCommandEditor(value);
	}

	const auto primary = PropertyMime::primaryValueMimeType(mimeTypes, ignoredValueMimeTypes());
	const auto mime = primary.trimmed().toLower();

	if (mime.startsWith(QLatin1String("image/"))) {
		if (auto* image = createImageEditor(value)) {
			return image;
		}
	}

	if (mime == QLatin1String(PropertyMimeType::kJson)) {
		return createMultilineEditor(formattedJson(value));
	}

	if (mime == QLatin1String(PropertyMimeType::kMultiline)
		|| mime == QLatin1String(PropertyMimeType::kXml)
		|| mime == QLatin1String("text/xml")
		|| mime == QLatin1String("text/x-ini")) {
		return createMultilineEditor(valueText(value, primary));
	}

	if (mime == QLatin1String(PropertyMimeType::kBoolean)) {
		return createBooleanEditor(value.toBool());
	}

	if (mime == QLatin1String(PropertyMimeType::kColor)) {
		return createColorEditor(value);
	}

	return createTextEditor(valueText(value, primary));
}

QWidget* SystemRouterNodeViewWidget::Private::createCommandEditor(const QVariant& value) {
	const auto raw = value.toString().trimmed();
	const auto linkToolTip = SystemRouterNodeViewWidget::tr("Ссылка на команду: %1")
		.arg(raw.isEmpty() ? SystemRouterNodeViewWidget::tr("не задана") : raw);

	if (commandService) {
		const auto command = commandService->command(raw);
		if (command.has_value()) {
			// Показываем текст скрипта, а не идентификатор блока: скрипт —
			// это и есть значение параметра-команды.
			auto* editor = createMultilineEditor(command->script);
			QStringList toolTip;
			if (!command->description.trimmed().isEmpty()) {
				toolTip.append(command->description);
			}
			if (command->version > 0) {
				toolTip.append(SystemRouterNodeViewWidget::tr("Версия: %1").arg(command->version));
			}
			toolTip.append(linkToolTip);
			editor->setToolTip(toolTip.join(QLatin1Char('\n')));
			return editor;
		}

		auto* editor = createTextEditor(SystemRouterNodeViewWidget::tr("Команда не найдена: %1")
			.arg(raw.isEmpty()
				? SystemRouterNodeViewWidget::tr("ссылка не задана")
				: raw));
		editor->setToolTip(linkToolTip);
		return editor;
	}

	auto* editor = createTextEditor(raw);
	editor->setToolTip(linkToolTip);
	return editor;
}

QWidget* SystemRouterNodeViewWidget::Private::createImageEditor(const QVariant& value) {
	const auto pixmap = pixmapForValue(value);
	if (pixmap.isNull()) {
		return nullptr;
	}

	auto* label = new QLabel(content);
	label->setObjectName(QStringLiteral("SystemRouterNodeViewImage"));
	label->setPixmap(pixmap.scaled(kImagePreviewWidth, kImagePreviewHeight,
		Qt::KeepAspectRatio, Qt::SmoothTransformation));
	label->setToolTip(SystemRouterNodeViewWidget::tr("Изображение %1×%2")
		.arg(pixmap.width()).arg(pixmap.height()));
	return label;
}

QWidget* SystemRouterNodeViewWidget::Private::createColorEditor(const QVariant& value) {
	const QColor color = value.metaType().id() == QMetaType::QColor
		? value.value<QColor>()
		: QColor(value.toString());

	auto* container = new QWidget(content);
	container->setObjectName(QStringLiteral("SystemRouterNodeViewColor"));
	auto* layout = new QHBoxLayout(container);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	auto* swatch = new QLabel(container);
	swatch->setObjectName(QStringLiteral("SystemRouterNodeViewColorSwatch"));
	swatch->setAutoFillBackground(true);
	swatch->setMinimumSize(24, 18);
	swatch->setMaximumHeight(18);
	if (color.isValid()) {
		auto palette = swatch->palette();
		palette.setColor(QPalette::Window, color);
		swatch->setPalette(palette);
	}
	layout->addWidget(swatch);

	auto* editor = new QLineEdit(container);
	editor->setObjectName(QStringLiteral("SystemRouterNodeViewValue"));
	editor->setReadOnly(true);
	editor->setText(color.isValid() ? color.name(QColor::HexArgb) : value.toString());
	editor->setCursorPosition(0);
	layout->addWidget(editor, 1);

	return container;
}

QWidget* SystemRouterNodeViewWidget::Private::createBooleanEditor(bool value) {
	auto* check = new QCheckBox(value
		? SystemRouterNodeViewWidget::tr("true")
		: SystemRouterNodeViewWidget::tr("false"), content);
	check->setObjectName(QStringLiteral("SystemRouterNodeViewValue"));
	check->setChecked(value);
	// Просмотр только читает: флажок отражает значение, но не меняется.
	check->setEnabled(false);
	return check;
}

QWidget* SystemRouterNodeViewWidget::Private::createTextEditor(const QString& text) {
	auto* editor = new QLineEdit(content);
	editor->setObjectName(QStringLiteral("SystemRouterNodeViewValue"));
	editor->setReadOnly(true);
	editor->setText(truncateForDisplay(text));
	editor->setCursorPosition(0);
	editor->setPlaceholderText(SystemRouterNodeViewWidget::tr("пусто"));
	if (!text.isEmpty()) {
		editor->setToolTip(text);
	}
	return editor;
}

QWidget* SystemRouterNodeViewWidget::Private::createMultilineEditor(const QString& text) {
	auto* editor = new QPlainTextEdit(content);
	editor->setObjectName(QStringLiteral("SystemRouterNodeViewValue"));
	editor->setReadOnly(true);
	editor->setPlainText(text);
	editor->setFixedHeight(kMultilineEditorHeight);
	return editor;
}

QString SystemRouterNodeViewWidget::Private::propertyTooltip(const QStringList& mimeTypes) const {
	QStringList lines;
	lines.append(mimeTypes.isEmpty()
		? SystemRouterNodeViewWidget::tr("Mime-типы не заданы")
		: SystemRouterNodeViewWidget::tr("Mime-типы: %1")
			.arg(mimeTypes.join(QStringLiteral(", "))));

	if (PropertyMime::isSecret(mimeTypes)) {
		lines.append(SystemRouterNodeViewWidget::tr("Секретный параметр: значение скрыто"));
	}

	return lines.join(QLatin1Char('\n'));
}

QString SystemRouterNodeViewWidget::Private::propertyClipboardText(const QString& name,
	const QStringList& mimeTypes) const {
	const auto value = node ? node->properties().value(name) : QVariant();

	// Для параметра-команды значение — текст скрипта из data_blobs, а не GUID.
	if (hasMime(mimeTypes, SystemRouterMime::kCommand) && commandService) {
		const auto command = commandService->command(value.toString().trimmed());
		if (command.has_value()) {
			return command->script;
		}
	}

	return valueToClipboardText(value);
}

void SystemRouterNodeViewWidget::Private::copyToClipboard(const QString& text,
	const QString& message) {
	if (auto* clipboard = QGuiApplication::clipboard()) {
		clipboard->setText(text);
	}

	status->setText(message);
	status->show();
	statusTimer->start();
	emit q->copied(text);
}

SystemRouterNodeViewWidget::SystemRouterNodeViewWidget(SystemRouterService* service,
	SystemRouterCommandService* commandService, QWidget* parent)
	: QWidget(parent)
	, d(std::make_unique<Private>(this)) {
	d->service = service;
	d->commandService = commandService;

	setObjectName(QStringLiteral("SystemRouterNodeViewWidget"));
	d->setupUi();
	d->rebuild();
}

SystemRouterNodeViewWidget::~SystemRouterNodeViewWidget() = default;

void SystemRouterNodeViewWidget::setNodeId(const QVariant& nodeId) {
	d->nodeId = nodeId;
	d->loadNode();
	d->rebuild();
}

QVariant SystemRouterNodeViewWidget::nodeId() const {
	return d->nodeId;
}

LazyTreeNodePtr SystemRouterNodeViewWidget::node() const {
	return d->node;
}

void SystemRouterNodeViewWidget::refresh() {
	d->loadNode();
	if (d->service && d->nodeId.isValid() && !d->nodeId.isNull() && !d->node) {
		emit errorOccurred(tr("Узел не найден: %1").arg(d->nodeId.toString()));
	}

	d->rebuild();
}

QStringList SystemRouterNodeViewWidget::propertyNames() const {
	return d->names;
}
