#include "ApplicationLayer/textures/textures_service.h"

class TexturesService::Private {
public:
	Private(TexturesService* parent) : q(parent) {
	}

	TexturesService* q;
	ImagesService* imagesService;
};


TexturesService::TexturesService(ImagesService* imagesService)
	: d(std::make_unique<Private>(this)) {
	d->imagesService = imagesService;
}

TexturesService::~TexturesService() = default;
