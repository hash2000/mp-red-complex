#pragma once
#include <memory>

class CommandController;
namespace BootstrapConfig {
struct Config;
}

/// Создаёт контексты приложения по bootstrap.yaml:
/// .store.<name> с настройками базы, WAL и списком SQL-миграций.
class BootstrapContextBuilder {
public:
	explicit BootstrapContextBuilder(CommandController* commandController);
	~BootstrapContextBuilder();

	void reload(const BootstrapConfig::Config& config);

private:
	class Private;
	std::unique_ptr<Private> d;
};
