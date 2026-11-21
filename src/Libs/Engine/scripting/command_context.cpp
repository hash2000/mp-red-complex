#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_context/context_interceptor.h"
#include "Libs/Engine/scripting/command_context/context_interceptor_factory.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/services/services_registry.h"

#include <QDebug>
#include <QList>
#include <QMap>

namespace {
/// Имя контекста, в котором живут документы, созданные другим контекстом.
constexpr const char* kDocumentsScope = "documents";
/// Имя корневого контекста системных настроек.
constexpr const char* kOptionsScope = "options";
/// Событие, которым завершается пакетное изменение переменных.
constexpr const char* kVariablesBatchComplete = "variables.batch.complete";
/// Имя глобального контекста
constexpr const char* kGlobalContextScope = ".";
}

class CommandContext::Private {
public:
	Private(CommandContext* parent) : q(parent) { }
	CommandContext* q;

	CommandController* commandController = nullptr;
	QMap<QString, std::weak_ptr<CommandContext>> contexts;
	std::shared_ptr<CommandContext> parentContext;
	QString name;
	QString nameSpace;

	/// Кэш контекста `documents`; слабая ссылка, чтобы не создавать цикл
	/// «родитель → ребёнок → родитель».
	std::weak_ptr<CommandContext> documentsContext;
	/// Кэш корневого контекста системных настроек `.options`.
	std::weak_ptr<CommandContext> optionsContext;

	QMap<QString, QVariant> variables;
	QMap<QString, QString> variableMimeTypes;
	CommandContext::VariablesMode variablesMode = CommandContext::VariablesMode::Lazy;

	/// Правила контекста; по умолчанию — стандартное поведение системы.
	/// Проверки interceptor приоритетнее variablesMode.
	std::unique_ptr<IContextInterceptor> interceptor;

	/// Пакетная обработка переменных: пока она объявлена, изменение
	/// отдельных переменных не рассылается, а копится в списке.
	bool variablesBatchActive = false;
	QStringList batchChangedVariables;

	void rememberChangedVariable(const QString& name);

	void snapshotParentVariables();
	std::shared_ptr<CommandContext> childByName(const QString& childName) const;
};

void CommandContext::Private::rememberChangedVariable(const QString& name) {
	if (batchChangedVariables.contains(name)) {
		return;
	}

	batchChangedVariables.append(name);
}


void CommandContext::Private::snapshotParentVariables() {
	if (!parentContext) {
		return;
	}

	const auto inherited = parentContext->variables();
	for (auto it = inherited.cbegin(); it != inherited.cend(); ++it) {
		if (!variables.contains(it.key())) {
			variables.insert(it.key(), it.value());
		}

		if (variableMimeTypes.contains(it.key())) {
			continue;
		}

		const auto mimeType = parentContext->variableMimeType(it.key());
		if (!mimeType.isEmpty()) {
			variableMimeTypes.insert(it.key(), mimeType);
		}
	}
}

CommandContext::CommandContext(CommandController* commandController, std::shared_ptr<CommandContext> parentContext, const QString& name, QObject* parent)
: d(std::make_unique<Private>(this))
, QObject(parent) {
	d->commandController = commandController;
	d->parentContext = parentContext;
	d->name = name;
	d->interceptor = createContextInterceptor(DocumentRule::Document);

	if (parentContext) {
		auto parentName = QString();
		if (!parentContext->isGlobalContext()) {
			parentName = parentContext->nameSpace();
		}
		d->nameSpace = QString("%1.%2")
			.arg(parentName)
			.arg(name);
	}
	else {
		d->nameSpace = ".";
		d->name = QString();
	}
}

CommandContext::~CommandContext() = default;

CommandController* CommandContext::commandController() const {
	return d->commandController;
}

QString CommandContext::name() const {
	return d->name;
}

QString CommandContext::nameSpace() const {
	return d->nameSpace;
}

std::shared_ptr<CommandContext> CommandContext::createScopedContext(const QString& name) {
	auto self = shared_from_this();
	auto scoped = std::make_shared<CommandContext>(d->commandController, self, name);
	d->commandController->globalContext()->registerContext(scoped->nameSpace(), scoped);
	return scoped;
}

std::shared_ptr<CommandContext> CommandContext::parentContext() const {
	return d->parentContext;
}

std::shared_ptr<CommandContext> CommandContext::globalContext() const {
	return d->commandController->globalContext();
}

bool CommandContext::isGlobalContext() const {
	return !d->parentContext;
}

void CommandContext::setInterceptor(std::unique_ptr<IContextInterceptor> interceptor) {
	if (!interceptor) {
		d->interceptor = createContextInterceptor(DocumentRule::Document);
		return;
	}

	d->interceptor = std::move(interceptor);

	// Режим переменных — часть правил контекста: interceptor задаёт его,
	// если роль требует конкретного режима (например, Mutable для `.options`).
	if (const auto mode = d->interceptor->variablesMode()) {
		setVariablesMode(*mode);
	}
}

const IContextInterceptor* CommandContext::interceptor() const {
	return d->interceptor.get();
}

std::shared_ptr<CommandContext> CommandContext::context(const QString& ns) const {
	if (ns == kGlobalContextScope) {
		return globalContext();
	}

	if (isGlobalContext()) {
		const auto it = d->contexts.constFind(ns);
		if (it == d->contexts.constEnd() || it->expired()) {
			return nullptr;
		}

		auto locked = it->lock();
		if (!locked) {
			return nullptr;
		}

		return locked;
	}

	return globalContext()->context(ns);
}

QList<std::shared_ptr<CommandContext>> CommandContext::childContexts() const {
	QList<std::shared_ptr<CommandContext>> result;
	const auto global = globalContext();
	if (!global) {
		return result;
	}
	for (auto it = global->d->contexts.cbegin(); it != global->d->contexts.cend(); it++) {
		const auto child = it.value().lock();
		if (child && child->parentContext().get() == this) {
			result.append(child);
		}
	}
	return result;
}

QStringList CommandContext::nameSpaces(bool alive) const {
	if (isGlobalContext()) {
		QStringList result;
		for (auto it = d->contexts.cbegin(); it != d->contexts.cend(); it++) {
			// По умолчанию мёртвые записи системе не видны: их убирает очистка.
			if (alive && it->expired()) {
				continue;
			}

			result << it.key();
		}

		return result;
	}

	return globalContext()->nameSpaces(alive);
}

void CommandContext::setVariablesMode(VariablesMode mode) {
	if (d->variablesMode == mode) {
		return;
	}

	d->variablesMode = mode;

	if (mode == VariablesMode::Atomic) {
		d->snapshotParentVariables();
	}
}

CommandContext::VariablesMode CommandContext::variablesMode() const {
	return d->variablesMode;
}

bool CommandContext::setVariable(const QString& name, const QVariant& value) {
	// Правила роли документа приоритетнее режима переменных.
	if (d->interceptor && !d->interceptor->canSetVariable(*this, name, value)) {
		qWarning() << "CommandContext::setVariable: variable" << name
			<< "is forbidden by document rules in" << d->nameSpace;
		return false;
	}

	if (d->variablesMode == VariablesMode::ReadOnly) {
		qWarning() << "CommandContext::setVariable: variable" << name
			<< "is read-only in" << d->nameSpace;
		return false;
	}

	// mutable: меняем общую переменную базового контекста, если она там есть.
	if (d->variablesMode == VariablesMode::Mutable &&
		!d->variables.contains(name) &&
		d->parentContext &&
		d->parentContext->hasVariable(name)) {
		return d->parentContext->setVariable(name, value);
	}

	const bool hadValue = d->variables.contains(name);
	const QVariant oldValue = hadValue ? d->variables.value(name) : QVariant();
	d->variables.insert(name, value);

	if (!hadValue || oldValue != value) {
		// В пакетном режиме сигналы отдельных переменных не рассылаются:
		// документы получат одно событие по завершении пакета.
		if (d->variablesBatchActive) {
			d->rememberChangedVariable(name);
		}
		else {
			emit variableChanged(d->nameSpace, name, oldValue, value);
		}
	}

	return true;
}

bool CommandContext::removeVariable(const QString& name) {
	if (d->interceptor && !d->interceptor->canRemoveVariable(*this, name)) {
		qWarning() << "CommandContext::removeVariable: variable" << name
			<< "is forbidden by document rules in" << d->nameSpace;
		return false;
	}

	if (d->variablesMode == VariablesMode::ReadOnly) {
		qWarning() << "CommandContext::removeVariable: context" << d->nameSpace
			<< "is read-only, variable" << name << "is not removed";
		return false;
	}

	// Удаляются только собственные переменные: унаследованные принадлежат
	// базовому контексту и не могут быть удалены отсюда.
	const auto it = d->variables.find(name);
	if (it == d->variables.end()) {
		return false;
	}

	const auto oldValue = it.value();
	d->variables.erase(it);
	d->variableMimeTypes.remove(name);

	if (d->variablesBatchActive) {
		d->rememberChangedVariable(name);
	}
	else {
		emit variableChanged(d->nameSpace, name, oldValue, QVariant());
	}

	return true;
}

void CommandContext::fireEvent(const QString& eventName) {
	// Правила роли документа приоритетнее режима переменных.
	if (d->interceptor && !d->interceptor->canFireEvent(*this, eventName)) {
		qWarning() << "CommandContext::fireEvent: event" << eventName
			<< "is forbidden by document rules in" << d->nameSpace;
		return;
	}

	// Отправка события сама по себе событием не является.
	if (eventName == variablesBatchCompleteEvent()) {
		qWarning() << "CommandContext::fireEvent: reserved event name" << eventName;
		return;
	}

	emit eventFired(eventName);
}

const QString& CommandContext::variablesBatchCompleteEvent() {
	static const QString event = QString::fromLatin1(kVariablesBatchComplete);
	return event;
}

const QString& CommandContext::documentSaveCompleteEvent() {
	static const QString event = QStringLiteral("document.save.complete");
	return event;
}

void CommandContext::beginVariablesBatch() {
	d->variablesBatchActive = true;
	d->batchChangedVariables.clear();
}

void CommandContext::endVariablesBatch() {
	if (!d->variablesBatchActive) {
		return;
	}

	d->variablesBatchActive = false;

	// Событие отправляется, пока список изменённых переменных ещё доступен:
	// подписчики читают его в обработчике.
	emit eventFired(variablesBatchCompleteEvent());

	d->batchChangedVariables.clear();
}

bool CommandContext::isVariablesBatchActive() const {
	return d->variablesBatchActive;
}

QStringList CommandContext::changedVariables() const {
	// Список существует только внутри пакета: вне его изменений не копится.
	if (!d->variablesBatchActive) {
		return { };
	}

	return d->batchChangedVariables;
}

bool CommandContext::hasVariable(const QString& name) const {
	if (d->variables.contains(name)) {
		return true;
	}

	if (d->variablesMode == VariablesMode::Atomic || !d->parentContext) {
		return false;
	}

	return d->parentContext->hasVariable(name);
}

QVariant CommandContext::variable(const QString& name, const QVariant& defaultValue) const {
	const auto it = d->variables.constFind(name);
	if (it != d->variables.constEnd()) {
		return it.value();
	}

	if (d->variablesMode == VariablesMode::Atomic || !d->parentContext) {
		return defaultValue;
	}

	return d->parentContext->variable(name, defaultValue);
}

QVariantMap CommandContext::variables() const {
	QVariantMap result;

	if (d->variablesMode != VariablesMode::Atomic && d->parentContext) {
		result = d->parentContext->variables();
	}

	for (auto it = d->variables.cbegin(); it != d->variables.cend(); ++it) {
		result.insert(it.key(), it.value());
	}

	return result;
}

QStringList CommandContext::variableNames() const {
	return variables().keys();
}

QVariantMap CommandContext::ownVariables() const {
	return d->variables;
}

void CommandContext::setVariableMimeType(const QString& name, const QString& mimeType) {
	if (mimeType.isEmpty()) {
		d->variableMimeTypes.remove(name);
		return;
	}

	d->variableMimeTypes.insert(name, mimeType);
}

QString CommandContext::variableMimeType(const QString& name, const QString& defaultValue) const {
	const auto it = d->variableMimeTypes.constFind(name);
	if (it != d->variableMimeTypes.constEnd()) {
		return it.value();
	}

	if (d->variablesMode == VariablesMode::Atomic || !d->parentContext) {
		return defaultValue;
	}

	return d->parentContext->variableMimeType(name, defaultValue);
}

void CommandContext::registerContext(const QString& ns, std::weak_ptr<CommandContext> context) {
	// Повторная регистрация после того, как прежний контекст был уничтожен
	// (реестр хранит слабые ссылки), — это норма, а не конфликт.
	if (d->contexts.value(ns).lock()) {
		qWarning() << "CommandContext::registerNamespace:" << ns << "allready defined. Revrite.";
	}

	d->contexts[ns] = context;
}

std::shared_ptr<CommandContext> CommandContext::Private::childByName(const QString& childName) const {
	for (const auto& child : q->childContexts()) {
		if (child->name() == childName) {
			return child;
		}
	}

	return nullptr;
}

std::shared_ptr<CommandContext> CommandContext::documentsContext() {
	if (auto cached = d->documentsContext.lock()) {
		return cached;
	}

	if (auto existing = d->childByName(QString::fromLatin1(kDocumentsScope))) {
		// Документы не могут менять системные параметры: их контекст
		// открыт только для чтения.
		existing->setVariablesMode(VariablesMode::ReadOnly);
		d->documentsContext = existing;
		return existing;
	}

	auto created = createScopedContext(QString::fromLatin1(kDocumentsScope));
	created->setVariablesMode(VariablesMode::ReadOnly);
	d->documentsContext = created;
	return created;
}

std::shared_ptr<CommandContext> CommandContext::optionsContext() {
	// `.options` — единственный корневой контекст настроек: из вложенного
	// контекста обращаемся к глобальному.
	if (!isGlobalContext()) {
		return globalContext()->optionsContext();
	}

	if (auto cached = d->optionsContext.lock()) {
		return cached;
	}

	auto context = d->childByName(QString::fromLatin1(kOptionsScope));
	if (!context) {
		context = createScopedContext(QString::fromLatin1(kOptionsScope));
	}

	// Привилегированный контекст системных настроек: документ с ролью
	// SystemOptions работает в нём в режиме Mutable и видит глобальные
	// значения системных параметров.
	context->setInterceptor(createContextInterceptor(DocumentRule::SystemOptions));
	d->optionsContext = context;
	return context;
}

std::shared_ptr<CommandContext> CommandContext::namedScope(const QString& scopeName) {
	if (scopeName.isEmpty() || scopeName == QLatin1String(kDocumentsScope)) {
		return documentsContext();
	}

	if (scopeName == QLatin1String(kOptionsScope)) {
		return globalContext()->optionsContext();
	}

	// Прочие корневые scope: создаются при первом обращении, правила
	// назначает interceptor, который их запросил.
	auto global = globalContext();
	if (auto existing = global->context(QStringLiteral(".%1").arg(scopeName))) {
		return existing;
	}

	return global->createScopedContext(scopeName);
}

std::shared_ptr<CommandContext> CommandContext::documentContext(const QString& documentId) const {
	if (documentId.isEmpty()) {
		return nullptr;
	}

	// У глобального контекста nameSpace равен ".", поэтому базовый префикс
	// строится так же, как в конструкторе дочернего контекста.
	const auto base = isGlobalContext() ? QString() : d->nameSpace;
	const auto ns = QString("%1.%2.%3")
		.arg(base, QString::fromLatin1(kDocumentsScope), documentId);

	auto context = globalContext()->context(ns);
	if (!context || !context->isDocumentContext()) {
		return nullptr;
	}

	return context;
}

std::shared_ptr<CommandContext> CommandContext::createDocumentContext(const QString& documentId,
	DocumentRule rule) {
	auto interceptor = createContextInterceptor(rule);
	if (!interceptor) {
		qWarning() << "CommandContext::createDocumentContext: unknown document rule" << documentRuleName(rule);
		return nullptr;
	}

	// Правила роли решают, в каком контексте создаётся документ.
	auto scope = namedScope(interceptor->scopeName());
	if (!scope) {
		return nullptr;
	}

	// Документ, который сам является контекстом scope (`.options`):
	// отдельный узел не создаётся, контекст получает правила роли.
	if (interceptor->documentLivesInScope()) {
		scope->setInterceptor(std::move(interceptor));
		return scope;
	}

	if (documentId.isEmpty()) {
		return nullptr;
	}

	if (auto existing = scope->d->childByName(documentId)) {
		return existing;
	}

	auto created = scope->createScopedContext(documentId);
	created->setInterceptor(std::move(interceptor));
	emit scope->documentAttached(documentId);
	return created;
}

bool CommandContext::isDocumentContext() const {
	if (!d->parentContext) {
		return false;
	}

	return d->parentContext->name() == QString::fromLatin1(kDocumentsScope);
}

QString CommandContext::documentId() const {
	return isDocumentContext() ? d->name : QString();
}

std::shared_ptr<CommandContext> CommandContext::owningDocumentContext() const {
	auto current = d->parentContext;
	while (current) {
		if (current->isDocumentContext()) {
			return current;
		}

		current = current->parentContext();
	}

	return nullptr;
}

void CommandContext::notifyDocumentAttached(const QString& documentId) {
	emit documentAttached(documentId);
}

void CommandContext::notifyDocumentActivated(const QString& documentId) {
	emit documentActivated(documentId);
}

void CommandContext::notifyDocumentDeactivated() {
	emit documentDeactivated();
}

void CommandContext::notifyDocumentInvoked(const QString& documentId) {
	emit documentInvoked(documentId);
}

int CommandContext::pruneDeadContexts() {
	auto global = globalContext();
	if (!global) {
		return 0;
	}

	const int removed = global->pruneDeadSubtree();
	if (removed > 0) {
		qDebug() << "CommandContext: removed" << removed << "dead contexts";
	}

	return removed;
}

int CommandContext::pruneDeadSubtree() {
	int removed = 0;

	// Реестр хранит слабые ссылки: если владелец контекст уничтожил,
	// запись остаётся «мёртвой» и её пора убрать.
	for (auto it = d->contexts.begin(); it != d->contexts.end(); ) {
		if (it->expired()) {
			it = d->contexts.erase(it);
			removed++;
		}
		else {
			it++;
		}
	}

	// У живых детей могут быть собственные реестры.
	for (const auto& child : childContexts()) {
		removed += child->pruneDeadSubtree();
	}

	return removed;
}

