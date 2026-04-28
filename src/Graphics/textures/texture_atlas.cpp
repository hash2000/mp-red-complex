#include "Graphics/textures/texture_atlas.h"
#include "Graphics/textures/uploaded_texture.h"
#include <qdebug.h>

class TextureAtlas::Private {
public:
	Private(TextureAtlas* parent, int tileSizeX, int tileSizeY)
		: q(parent)
		, tileSizeX(tileSizeX)
		, tileSizeY(tileSizeY) {
	}

	TextureAtlas* q;
	int tileSizeX;
	int tileSizeY;
	int tilesCountX = 0;
	int tilesCountY = 0;
	bool useMipMaps = true;
	bool useMipMapsSmoothing = true;
	TextureFilter textureFilter = TextureFilter::Linear;
	std::shared_ptr<UploadedTexture> texture;
};

TextureAtlas::TextureAtlas(int tileSizeX, int tileSizeY)
	: d(std::make_unique<Private>(this, tileSizeX, tileSizeY)) {
}

TextureAtlas::~TextureAtlas() = default;


bool TextureAtlas::load(std::shared_ptr<UploadedTexture> texture, int tilesCountX, int tilesCountY) {
	if (!texture) {
		qWarning() << "TextureAtlas: texture is null";
		return false;
	}

	d->tilesCountX = tilesCountX;
	d->tilesCountY = tilesCountY;
	d->tileSizeX = texture->width() / tilesCountX;
	d->tileSizeY = texture->height() / tilesCountY;
	d->texture = texture;

	return true;
}

TextureRegion TextureAtlas::getRegion(int tileX, int tileY) const {
	if (!d->texture || tileX < 0 || tileX >= d->tilesCountX || tileY < 0 || tileY >= d->tilesCountY) {
		qWarning() << "TextureAtlas: invalid tile coordinates" << tileX << tileY;
		return TextureRegion();
	}

	float u1 = static_cast<float>(tileX) / d->tilesCountX;
	float u2 = static_cast<float>(tileX + 1) / d->tilesCountX;
	float v1 = 1.0f - static_cast<float>(tileY + 1) / d->tilesCountY;
	float v2 = 1.0f - static_cast<float>(tileY) / d->tilesCountY;

	return TextureRegion(u1, v1, u2, v2);
}

void TextureAtlas::bind() const {
	if (d->texture) {
		d->texture->bind();
	}
}

void TextureAtlas::unbind() const {
	if (d->texture) {
		d->texture->unbind();
	}
}

int TextureAtlas::tileSizeX() const {
	return d->tileSizeX;
}

int TextureAtlas::tileSizeY() const {
	return d->tileSizeY;
}

int TextureAtlas::tilesCountX() const {
	return d->tilesCountX;
}

int TextureAtlas::tilesCountY() const {
	return d->tilesCountY;
}

bool TextureAtlas::isLoaded() const {
	return d->texture != nullptr;
}

const UploadedTexture* TextureAtlas::texture() const {
	return d->texture ? d->texture.get() : nullptr;
}
