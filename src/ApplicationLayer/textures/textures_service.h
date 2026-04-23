#pragma once
#include <memory>

class ImagesService;

class TexturesService {
public:
	TexturesService(ImagesService* imagesService);
	~TexturesService();



private:
	class Private;
	std::unique_ptr<Private> d;
};
