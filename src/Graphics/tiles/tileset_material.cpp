#include "Graphics/tiles/tileset_material.h"
#include "Graphics/textures/texture_atlas.h"

class TilesetMaterial::Private {
public:
    Private(TilesetMaterial* parent) : q(parent) {}

    TilesetMaterial* q;
    std::shared_ptr<TextureAtlas> textureAtlas;
};

TilesetMaterial::TilesetMaterial() 
    : d(std::make_unique<Private>(this)) {
    materialType = Type::Tileset;
    materialName = "TilesetMaterial";
}

TilesetMaterial::~TilesetMaterial() = default;

void TilesetMaterial::setTextureAtlas(std::shared_ptr<TextureAtlas> atlas) {
    d->textureAtlas = atlas;
}

std::shared_ptr<TextureAtlas> TilesetMaterial::textureAtlas() const {
    return d->textureAtlas;
}

bool TilesetMaterial::initialize() {
    if (!Material::initialize()) {
        return false;
    }

    shaderProgram = std::make_unique<QOpenGLShaderProgram>();

    // Вершинный шейдер с поддержкой пермутаций
    QString vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPosition;
        layout (location = 1) in vec2 aTexCoord;
        
        uniform mat4 uProjection;
        uniform mat4 uView;
        uniform float zLevel;
        
        // Пермутации
        #ifdef USE_ANIMATION
            uniform float uTime;
            uniform vec2 uAnimationOffset;
        #endif
        
        out vec2 vTexCoord;
        
        void main() {
            vTexCoord = aTexCoord;
            
            #ifdef USE_ANIMATION
                vTexCoord += uAnimationOffset * sin(uTime);
            #endif
            
            gl_Position = uProjection * uView * vec4(aPosition.x, zLevel, aPosition.y, 1.0);
        }
    )";

    // Фрагментный шейдер с поддержкой пермутаций
    QString fragmentSource = R"(
        #version 330 core
        in vec2 vTexCoord;
        uniform sampler2D uTexture;
        
        // Пермутации
        #ifdef USE_ALPHA_TEST
            uniform float uAlphaThreshold;
        #endif
        
        #ifdef USE_COLOR_TINT
            uniform vec4 uColorTint;
        #endif
        
        out vec4 FragColor;
        
        void main() {
            vec4 texColor = texture(uTexture, vTexCoord);
            
            #ifdef USE_ALPHA_TEST
                if (texColor.a < uAlphaThreshold)
                    discard;
            #else
                if (texColor.a < 0.1)
                    discard;
            #endif
            
            #ifdef USE_COLOR_TINT
                texColor *= uColorTint;
            #endif
            
            FragColor = texColor;
        }
    )";

    if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        qWarning() << "TilesetMaterial: failed to compile vertex shader:" << shaderProgram->log();
        return false;
    }

    if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        qWarning() << "TilesetMaterial: failed to compile fragment shader:" << shaderProgram->log();
        return false;
    }

    if (!shaderProgram->link()) {
        qWarning() << "TilesetMaterial: failed to link shader program:" << shaderProgram->log();
        return false;
    }

    setupShaderPermutations();
    return true;
}

void TilesetMaterial::bind() {
    Material::bind();
    
    // Устанавливаем текстуру
    if (d->textureAtlas && d->textureAtlas->isLoaded()) {
        d->textureAtlas->bind();
        shaderProgram->setUniformValue("uTexture", 0);
    }
}

void TilesetMaterial::unbind() {
    if (d->textureAtlas && d->textureAtlas->isLoaded()) {
        d->textureAtlas->unbind();
    }
    Material::unbind();
}

size_t TilesetMaterial::sortKey() const {
    // Ключ сортировки включает тип и ID текстуры
    size_t key = static_cast<size_t>(materialType);
    if (d->textureAtlas) {
        // Используем адрес TextureAtlas как уникальный идентификатор
        key ^= reinterpret_cast<size_t>(d->textureAtlas.get());
    }
    return key;
}

bool TilesetMaterial::isCompatibleWith(const Material& other) const {
    if (!Material::isCompatibleWith(other)) {
        return false;
    }
    
    const auto* otherTileset = dynamic_cast<const TilesetMaterial*>(&other);
    if (!otherTileset) {
        return false;
    }
    
    // Совместимы если используют одну и ту же текстуру
    return d->textureAtlas == otherTileset->d->textureAtlas;
}

void TilesetMaterial::setupShaderPermutations() {
    // В данной реализации все пермутации уже встроены в шейдер
    // Можно активировать/деактивировать через униформы
    shaderProgram->setUniformValue("uAlphaThreshold", 0.1f);
    shaderProgram->setUniformValue("uColorTint", 1.0f, 1.0f, 1.0f, 1.0f);
}

void TilesetMaterial::bindSpecific() {
    // Специфичная привязка для тайлсета
}

void TilesetMaterial::unbindSpecific() {
    // Специфичная отвязка для тайлсета
}
