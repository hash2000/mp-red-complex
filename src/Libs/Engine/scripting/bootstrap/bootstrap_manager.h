#pragma once
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"

#include <memory>

class Resources;

class BootstrapManager {
public:
	BootstrapManager(Resources* resources);
	virtual ~BootstrapManager();

	bool reload();

	/// Конфигурация, загруженная из bootstrap.yaml (тема и описываемое приложение).
	const BootstrapConfig::Config& config() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
