#include "Graphics/textures/texture_atlas.h"
#include "Graphics/textures/uploaded_texture.h"
#include <qdebug.h>

class TextureAtlas::Private {
public:
	Private(TextureAtlas* parent)
		: q(parent) {
	}

	TextureAtlas* q;
	int tileSizeX = 0;
	int tileSizeY = 0;
	int tilesCountX = 0;
	int tilesCountY = 0;
	bool useMipMaps = true;
	bool useMipMapsSmoothing = true;
	TextureFilter textureFilter = TextureFilter::Linear;
	std::shared_ptr<UploadedTexture> texture;
};

TextureAtlas::TextureAtlas()
	: d(std::make_unique<Private>(this)) {
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
	return d->texture != nullptr && d->texture->isLoaded();
}

const UploadedTexture* TextureAtlas::texture() const {
	return d->texture ? d->texture.get() : nullptr;
}

TileData TextureAtlas::getTile(int tileX, int tileY) const {
	if (!isLoaded()) {
		return TileData::empty();
	}

	if (tileX < 0 || tileX >= d->tilesCountX || tileY < 0 || tileY >= d->tilesCountY) {
		return TileData::empty();
	}

	TileData data;
	data.tileId = tileY * d->tilesCountX + tileX;
	data.region = getRegion(tileX, tileY);
	return data;
}

TileData TextureAtlas::getTileById(int tileId) const {
	if (tileId < 0 || tileId >= totalTiles()) {
		return TileData::empty();
	}

	int tileX = tileId % d->tilesCountX;
	int tileY = tileId / d->tilesCountX;
	return getTile(tileX, tileY);
}

int TextureAtlas::totalTiles() const {
	return d->tilesCountX * d->tilesCountY;
}
