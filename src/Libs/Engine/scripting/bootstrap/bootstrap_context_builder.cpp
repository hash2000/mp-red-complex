#include "Libs/Engine/scripting/bootstrap/bootstrap_context_builder.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QDebug>

#include <map>
#include <utility>

namespace {

constexpr const char* kStoresContextName = "store";
constexpr const char* kStoresContextNamespace = ".store";
constexpr const char* kWalCheckIntervalVariable = "wal.checkInterval";
constexpr const char* kWalMaxSizeMbVariable = "wal.maxSizeMb";

} // namespace

class BootstrapContextBuilder::Private {
public:
	Private(BootstrapContextBuilder* parent, CommandController* commandController)
		: q(parent)
		, commandController(commandController) {}

	BootstrapContextBuilder* q;
	CommandController* commandController;

	// Владение контекстами: в CommandController они хранятся слабыми ссылками.
	std::shared_ptr<CommandContext> storesContext;
	std::map<QString, std::shared_ptr<CommandContext>> stores;
};

BootstrapContextBuilder::BootstrapContextBuilder(CommandController* commandController)
	: d(std::make_unique<Private>(this, commandController)) {
}

BootstrapContextBuilder::~BootstrapContextBuilder() = default;

void BootstrapContextBuilder::reload(const BootstrapConfig::Config& config) {
	if (!d->commandController) {
		qWarning() << "BootstrapContextBuilder: command controller is not available";
		return;
	}

	const auto globalContext = d->commandController->globalContext();
	if (!globalContext) {
		qWarning() << "BootstrapContextBuilder: global context is not available";
		return;
	}

	// При первом вызове создаём корневой контекст `.store`.
	if (!d->storesContext) {
		d->storesContext = globalContext->createScopedContext(QString::fromLatin1(kStoresContextName));
	}

	std::map<QString, std::shared_ptr<CommandContext>> actualStores;

	for (const auto& store : config.stores) {
		if (store.name.isEmpty()) {
			qWarning() << "BootstrapContextBuilder: store without a name";
			continue;
		}

		// Переиспользуем существующий контекст или создаём новый.
		const auto existing = d->stores.find(store.name);
		auto context = (existing != d->stores.end())
			? existing->second
			: d->storesContext->createScopedContext(store.name);

		// Обновляем переменные из bootstrap.yaml: вначале открываем для записи (режим Lazy),
		// устанавливаем значения, затем закрываем для дальнейших изменений (режим ReadOnly).
		context->setVariablesMode(CommandContext::VariablesMode::Lazy);
		context->setVariable("path", store.path);
		context->setVariable("encrypted", store.encrypted);
		QVariantList migrations;
		for (const auto& migration : store.migrations) {
			QVariantMap item;
			item.insert("version", QVariant::fromValue(migration.version));
			item.insert("description", migration.description);
			item.insert("up", migration.up);
			item.insert("down", migration.down);
			migrations.append(item);
		}
		context->setVariable("migrations", migrations);
		context->setVariable(QString::fromLatin1(kWalCheckIntervalVariable), store.wal.checkInterval);
		context->setVariable(QString::fromLatin1(kWalMaxSizeMbVariable), store.wal.maxSizeMb);
		context->setVariablesMode(CommandContext::VariablesMode::ReadOnly);

		actualStores[store.name] = context;

		qDebug() << "Bootstrap context:" << context->nameSpace()
			<< kWalCheckIntervalVariable << store.wal.checkInterval
			<< kWalMaxSizeMbVariable << store.wal.maxSizeMb;
	}

	// Хранилища, которых больше нет в конфигурации, отпускаем.
	d->stores = std::move(actualStores);
}
