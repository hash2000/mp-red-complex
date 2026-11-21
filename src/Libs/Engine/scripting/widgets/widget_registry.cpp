#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"
#include "Libs/Engine/ui/doc_widget_stub.h"
#include <QDebug>

#include <map>

WidgetTypeHandler::WidgetTypeHandler(const QString& className, const QString& xtype, bool singletone,
	const QString& title, DocumentRule rule)
	: className(className)
	, xtype(xtype)
	, singletone(singletone)
	, title(title)
	, rule(rule) {
}

class WidgetRegistry::Private {
public:
	std::map<QString, WidgetFactory> factoryByXtype;
	std::map<QString, WidgetFactory> factoryByClass;
	std::map<QString, WidgetTypeHandler> typeByXtype;
	std::map<QString, QString> xtypeByClass;
	std::map<QString, std::unique_ptr<IDocumentMetadata>> metadataByClass;

	std::optional<WidgetTypeHandler> typeInfo(const QString& type) const {
		const auto byXtype = typeByXtype.find(type);
		if (byXtype != typeByXtype.end()) {
			return byXtype->second;
		}

		const auto byClass = xtypeByClass.find(type);
		if (byClass == xtypeByClass.end()) {
			return std::nullopt;
		}

		const auto resolved = typeByXtype.find(byClass->second);
		if (resolved == typeByXtype.end()) {
			return std::nullopt;
		}

		return resolved->second;
	}
};

WidgetRegistry::WidgetRegistry() : d(std::make_unique<Private>()) {
}

WidgetRegistry::~WidgetRegistry() = default;

void WidgetRegistry::registerFactory(const WidgetTypeHandler& type, WidgetFactory factory) {
	if (type.className.isEmpty() || type.xtype.isEmpty()) {
		qWarning() << "WidgetRegistry: empty widget class or xtype";
		return;
	}

	if (d->factoryByXtype.find(type.xtype) != d->factoryByXtype.end()) {
		qWarning() << "WidgetRegistry: Widget xtype" << type.xtype << "allready registered";
		return;
	}

	if (d->factoryByClass.find(type.className) != d->factoryByClass.end()) {
		qWarning() << "WidgetRegistry: Widget class" << type.className << "allready registered";
		return;
	}

	if (!factory) {
		qWarning() << "WidgetRegistry: empty factory for" << type.xtype;
		return;
	}

	// Подпись по умолчанию: если регистрация её не задала, показываем xtype.
	WidgetTypeHandler normalized = type;
	if (normalized.title.isEmpty()) {
		normalized.title = normalized.xtype;
	}

	d->factoryByXtype.emplace(normalized.xtype, factory);
	d->factoryByClass.emplace(normalized.className, factory);
	d->typeByXtype.emplace(normalized.xtype, normalized);
	d->xtypeByClass.emplace(normalized.className, normalized.xtype);
}

QList<WidgetTypeHandler> WidgetRegistry::types() const {
	QList<WidgetTypeHandler> result;
	result.reserve(static_cast<int>(d->typeByXtype.size()));
	for (const auto& entry : d->typeByXtype) {
		result.append(entry.second);
	}
	return result;
}

std::optional<WidgetTypeHandler> WidgetRegistry::typeInfo(const QString& type) const {
	return d->typeInfo(type);
}

QString WidgetRegistry::documentId(const QString& type, const QString& requestedId) const {
	const auto info = d->typeInfo(type);
	if (info && info->singletone) {
		return info->xtype;
	}

	return requestedId;
}

void WidgetRegistry::registerDocumentMetadata(const QString& className,
	std::unique_ptr<IDocumentMetadata> metadata) {
	if (className.isEmpty() || !metadata) {
		qWarning() << "WidgetRegistry: empty document metadata for class" << className;
		return;
	}

	if (d->metadataByClass.find(className) != d->metadataByClass.end()) {
		qWarning() << "WidgetRegistry: document metadata for class" << className
			<< "allready registered";
		return;
	}

	d->metadataByClass.emplace(className, std::move(metadata));
}

const IDocumentMetadata* WidgetRegistry::documentMetadata(const QString& type) const {
	const auto info = d->typeInfo(type);
	const auto className = info ? info->className : type;
	const auto it = d->metadataByClass.find(className);
	return it != d->metadataByClass.end() ? it->second.get() : nullptr;
}

QWidget* WidgetRegistry::buildWidget(const QString& type, const WidgetBuildHandler& handler) {
	WidgetFactory* factory = nullptr;

	const auto itxtype = d->factoryByXtype.find(type);
	if (itxtype != d->factoryByXtype.end()) {
		factory = &itxtype->second;
	}
	else {
		const auto itclass = d->factoryByClass.find(type);
		if (itclass == d->factoryByClass.end()) {
			return nullptr;
		}
		factory = &itclass->second;
	}

	// Фабрика получает описание типа: недостающие значения по умолчанию
	// (title) и имя singletone-документа берутся из регистрации; цвет и глиф
	// вкладки задаёт тема.
	WidgetBuildHandler effective = handler;
	const auto info = d->typeInfo(type);
	if (info) {
		if (effective.className.isEmpty()) {
			effective.className = info->className;
		}
		if (effective.xtype.isEmpty()) {
			effective.xtype = info->xtype;
		}
		if (effective.title.isEmpty()) {
			effective.title = info->title;
		}
		effective.singletone = info->singletone;
		if (info->singletone) {
			effective.name = info->xtype;
		}
	}

	return (*factory)(type, effective);
}

DocumentWidget* WidgetRegistry::build(const QString& type, const WidgetBuildHandler& handler) {
	QWidget* content = buildWidget(type, handler);
	if (!content) {
		return nullptr;
	}

	// Любой документ — как минимум DocumentWidget: у произвольного QWidget
	// нет контекста документа и корректного жизненного цикла при закрытии.
	if (auto* document = qobject_cast<DocumentWidget*>(content)) {
		return document;
	}

	return new DocumentWidgetStub(content);
}
