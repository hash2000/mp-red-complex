#pragma once
#include "Graphics/tiles/material.h"
#include <memory>

class TextureAtlas;

/// Материал для рендеринга тайлсетов с текстурным атласом
class TilesetMaterial : public Material {
public:
    TilesetMaterial();
    ~TilesetMaterial();

    /// Установить текстуру атласа
    void setTextureAtlas(std::shared_ptr<TextureAtlas> atlas);

    /// Получить текстуру атласа
    std::shared_ptr<TextureAtlas> textureAtlas() const;

    bool initialize() override;
    void bind() override;
    void unbind() override;

    size_t sortKey() const override;
    bool isCompatibleWith(const Material& other) const override;

private:
    class Private;
    std::unique_ptr<Private> d;

    void setupShaderPermutations() override;
    void bindSpecific() override;
    void unbindSpecific() override;
};
