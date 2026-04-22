#pragma once
#include "DataLayer/images/i_images_data_provider.h"
#include <memory>

class Resources;

class ImagesDataProviderJsonImpl : public IImagesDataProvider {
public:
	explicit ImagesDataProviderJsonImpl(Resources* resources);
	~ImagesDataProviderJsonImpl() override;

	std::optional<QPixmap> load(const QString& path, ImageType type = ImageType::Icon) const override;
	QStringList list(ImageType type) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
