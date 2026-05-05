#pragma once
#include "DataLayer/images/i_images_data_provider.h"
#include <QString>
#include <memory>

class ImagesService;
class UploadedTexture;
class UploadedTextureProperties;

inline constexpr const char* kDefaultTextureTag = "default";

class TexturesService {
public:
	TexturesService(ImagesService* imagesService);
	~TexturesService();

	std::shared_ptr<UploadedTexture> upload(
		const QString& path,
		const UploadedTextureProperties& properties,
		ImageType type = ImageType::Icon,
		const QString& tag = kDefaultTextureTag);

	void clear(const QString& tag);

private:
	class Private;
	std::unique_ptr<Private> d;
};
