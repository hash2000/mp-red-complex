#include "Libs/Engine/ui/doc_tab_panel/document_tabs_order.h"
#include "Libs/Engine/scripting/system_options/system_option.h"
#include "Libs/Engine/scripting/system_options/system_options_registry.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <QDebug>
#include <QMap>

#include <vector>

namespace {
/// Идентификатор правила ручной сортировки; оно же правило по умолчанию.
constexpr const char* kManualTabsOrder = "manual";
/// Идентификатор правила группировки по цвету вкладки.
constexpr const char* kColorTabsOrder = "color";
/// Идентификатор правила группировки по типу документа.
constexpr const char* kXtypeTabsOrder = "xtype";

/// Ручная сортировка: одна группа без заголовка, порядок задаёт пользователь
/// перетаскиванием вкладок мышью.
class ManualTabsOrderStrategy final : public IDocumentTabsOrderStrategy {
public:
	QString id() const override {
		return QString::fromLatin1(kManualTabsOrder);
	}

	QString title() const override {
		return QStringLiteral("Ручная сортировка");
	}

	bool supportsManualMove() const override {
		return true;
	}

	QList<DocumentTabGroup> arrange(const QList<DocumentWidget*>& documents) const override {
		DocumentTabGroup group;
		group.documents = documents;
		return { group };
	}
};

/// Группировка по цвету вкладки: цветные группы сортируются по имени цвета,
/// вкладки без цвета попадают в группу «Без цвета» в конце.
class ColorTabsOrderStrategy final : public IDocumentTabsOrderStrategy {
public:
	QString id() const override {
		return QString::fromLatin1(kColorTabsOrder);
	}

	QString title() const override {
		return QStringLiteral("Группировка по цвету");
	}

	bool supportsManualMove() const override {
		return false;
	}

	QList<DocumentTabGroup> arrange(const QList<DocumentWidget*>& documents) const override {
		QMap<QString, QList<DocumentWidget*>> groups;
		QList<DocumentWidget*> uncolored;
		for (auto* document : documents) {
			if (!document) {
				continue;
			}

			const auto color = document->color();
			if (!color.isValid()) {
				uncolored << document;
				continue;
			}

			groups[color.name()] << document;
		}

		QList<DocumentTabGroup> result;
		for (auto it = groups.cbegin(); it != groups.cend(); it++) {
			DocumentTabGroup group;
			group.title = it.key();
			group.color = QColor(it.key());
			group.documents = it.value();
			result << group;
		}

		if (!uncolored.isEmpty()) {
			DocumentTabGroup group;
			group.title = QStringLiteral("Без цвета");
			group.documents = uncolored;
			result << group;
		}

		return result;
	}
};

/// Группировка по типу документа (xtype): группы сортируются по имени типа,
/// документы без типа попадают в последнюю группу.
class XtypeTabsOrderStrategy final : public IDocumentTabsOrderStrategy {
public:
	QString id() const override {
		return QString::fromLatin1(kXtypeTabsOrder);
	}

	QString title() const override {
		return QStringLiteral("Группировка по типу");
	}

	bool supportsManualMove() const override {
		return false;
	}

	QList<DocumentTabGroup> arrange(const QList<DocumentWidget*>& documents) const override {
		QMap<QString, QList<DocumentWidget*>> groups;
		QList<DocumentWidget*> untyped;
		for (auto* document : documents) {
			if (!document) {
				continue;
			}

			const auto xtype = document->xtype();
			if (xtype.isEmpty()) {
				untyped << document;
				continue;
			}

			groups[xtype] << document;
		}

		QList<DocumentTabGroup> result;
		for (auto it = groups.cbegin(); it != groups.cend(); it++) {
			DocumentTabGroup group;
			group.title = it.key();
			group.documents = it.value();
			result << group;
		}

		if (!untyped.isEmpty()) {
			DocumentTabGroup group;
			group.title = QStringLiteral("Без типа");
			group.documents = untyped;
			result << group;
		}

		return result;
	}
};

} // namespace

class DocumentTabsOrderRegistry::Private {
public:
	std::vector<std::unique_ptr<IDocumentTabsOrderStrategy>> strategies;
};

DocumentTabsOrderRegistry::DocumentTabsOrderRegistry()
	: d(std::make_unique<Private>()) {
	// Встроенные правила. Новое правило добавляется здесь одной строкой.
	registerStrategy(std::make_unique<ManualTabsOrderStrategy>());
	registerStrategy(std::make_unique<ColorTabsOrderStrategy>());
	registerStrategy(std::make_unique<XtypeTabsOrderStrategy>());
}

DocumentTabsOrderRegistry::~DocumentTabsOrderRegistry() = default;

void DocumentTabsOrderRegistry::registerStrategy(std::unique_ptr<IDocumentTabsOrderStrategy> strategy) {
	if (!strategy || strategy->id().isEmpty()) {
		return;
	}

	if (this->strategy(strategy->id())) {
		qWarning() << "DocumentTabsOrderRegistry: strategy" << strategy->id()
			<< "allready registered";
		return;
	}

	d->strategies.push_back(std::move(strategy));
}

QList<IDocumentTabsOrderStrategy*> DocumentTabsOrderRegistry::strategies() const {
	QList<IDocumentTabsOrderStrategy*> result;
	result.reserve(static_cast<int>(d->strategies.size()));
	for (const auto& strategy : d->strategies) {
		result.append(strategy.get());
	}

	return result;
}

IDocumentTabsOrderStrategy* DocumentTabsOrderRegistry::strategy(const QString& id) const {
	for (const auto& strategy : d->strategies) {
		if (strategy->id() == id) {
			return strategy.get();
		}
	}

	return nullptr;
}

QStringList DocumentTabsOrderRegistry::ids() const {
	QStringList result;
	result.reserve(static_cast<int>(d->strategies.size()));
	for (const auto& strategy : d->strategies) {
		result << strategy->id();
	}

	return result;
}

QString DocumentTabsOrderRegistry::defaultId() const {
	return QString::fromLatin1(kManualTabsOrder);
}

void registerDocumentTabsOrderOption(SystemOptionsRegistry& registry) {
	DocumentTabsOrderRegistry strategies;

	SystemOption option;
	option.name = QString::fromLatin1(SystemOptions::kDocumentsTabsOrder);
	option.title = QStringLiteral("Сортировка вкладок документов");
	option.description = QStringLiteral(
		"Правило сортировки и группировки вкладок в панели документов. "
		"Действует одновременно только одно правило.");
	option.type = SystemOptionType::Enum;
	option.defaultValue = strategies.defaultId();
	for (auto* strategy : strategies.strategies()) {
		option.values << strategy->id();
		option.valueTitles << strategy->title();
	}

	registry.registerOption(option);
}
