#pragma once
#include "DataLayer/images/i_images_data_provider.h"
#include <QString>
#include <memory>

class ImagesService;
class UploadedTexture;
struct UploadedTextureProperties;

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



private:
	class Private;
	std::unique_ptr<Private> d;
};
