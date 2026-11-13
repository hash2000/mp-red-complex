#include "Libs/Engine/scripting/bootstrap/bootstrap_context_builder.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"

class BootstrapContextBuilder::Private {
public:
	Private(BootstrapContextBuilder* parent, CommandController* commandController)
		: q(parent)
		, commandController(commandController) {}

	BootstrapContextBuilder* q;
	CommandController* commandController;

};

BootstrapContextBuilder::BootstrapContextBuilder(CommandController* commandController)
	: d(std::make_unique<Private>(this, commandController)) {
}

BootstrapContextBuilder::~BootstrapContextBuilder() = default;

void BootstrapContextBuilder::reload(const BootstrapConfig::Config& config) {
	auto storesContext = d->commandController->globalContext()->createScopedContext("stores");

}
