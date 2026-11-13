#pragma once
#include <memory>

class CommandController;
namespace BootstrapConfig {
struct Config;
}

class BootstrapContextBuilder {
public:
public:
	explicit BootstrapContextBuilder(CommandController* commandController);
	~BootstrapContextBuilder();

	void reload(const BootstrapConfig::Config& config);

private:
	class Private;
	std::unique_ptr<Private> d;
};
