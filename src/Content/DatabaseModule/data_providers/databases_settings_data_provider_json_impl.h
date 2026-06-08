#pragma once
#include "Content/DatabaseModule/data_providers/i_databases_settings_data_provider.h"
#include <memory>

class Resources;

class DatabaseSettingsDataProviderJsonImpl : public IDatabaseSettingsDataProvider {
public:
	DatabaseSettingsDataProviderJsonImpl(Resources* resources);
	~DatabaseSettingsDataProviderJsonImpl() override;

	std::optional<Settings> get(const QString& name) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
