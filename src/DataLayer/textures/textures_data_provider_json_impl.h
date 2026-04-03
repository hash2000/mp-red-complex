#pragma once
#include "DataLayer/textures/i_textures_data_provider.h"
#include <memory>

class Resources;

class TexturesDataProviderJsonImpl : public ITexturesDataProvider {
public:
	explicit TexturesDataProviderJsonImpl(Resources* resources);
	~TexturesDataProviderJsonImpl() override;

	std::optional<QPixmap> loadTexture(const QString& path, TextureType type = TextureType::Icon) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
