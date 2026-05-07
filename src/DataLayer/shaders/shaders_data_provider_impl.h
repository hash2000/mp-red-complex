#pragma once
#include "DataLayer/shaders/i_shaders_data_provider.h"
#include <memory>

class Resources;

class ShadersDataProviderLocalImpl : public IShadersDataProvider {
public:
	ShadersDataProviderLocalImpl(Resources* resources);
	~ShadersDataProviderLocalImpl();

	std::optional<QString> LoadShader(const QString& path) override;
private:
	class Private;
	std::unique_ptr<Private> d;
};
