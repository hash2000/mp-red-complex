#include "Graphics/tiles/border_material.h"

class BorderMaterial::Private {
public:
    Private(BorderMaterial* parent) : q(parent) {}

    BorderMaterial* q;
    QColor color = QColor(255, 0, 0, 255);
};

BorderMaterial::BorderMaterial() 
    : d(std::make_unique<Private>(this)) {
    materialType = Type::Border;
    materialName = "BorderMaterial";
}

BorderMaterial::~BorderMaterial() = default;

void BorderMaterial::setColor(const QColor& color) {
    d->color = color;
    if (shaderProgram) {
        shaderProgram->setUniformValue("uColor", 
            color.redF(), color.greenF(), color.blueF(), color.alphaF());
    }
}

QColor BorderMaterial::color() const {
    return d->color;
}

bool BorderMaterial::initialize() {
    if (!Material::initialize()) {
        return false;
    }

    shaderProgram = std::make_unique<QOpenGLShaderProgram>();

    // Вершинный шейдер для рамок (без текстурных координат)
    QString vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPosition;
        
        uniform mat4 uProjection;
        uniform mat4 uView;
        uniform float zLevel;
        
        void main() {
            gl_Position = uProjection * uView * vec4(aPosition.x, zLevel, aPosition.y, 1.0);
        }
    )";

    // Фрагментный шейдер для рамок (только цвет)
    QString fragmentSource = R"(
        #version 330 core
        uniform vec4 uColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = uColor;
        }
    )";

    if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexSource)) {
        qWarning() << "BorderMaterial: failed to compile vertex shader:" << shaderProgram->log();
        return false;
    }

    if (!shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource)) {
        qWarning() << "BorderMaterial: failed to compile fragment shader:" << shaderProgram->log();
        return false;
    }

    if (!shaderProgram->link()) {
        qWarning() << "BorderMaterial: failed to link shader program:" << shaderProgram->log();
        return false;
    }

    setupShaderPermutations();
    setColor(d->color);
    return true;
}

void BorderMaterial::bind() {
    Material::bind();
}

void BorderMaterial::unbind() {
    Material::unbind();
}

size_t BorderMaterial::sortKey() const {
    // Ключ сортировки включает тип и цвет
    size_t key = static_cast<size_t>(materialType);
    key ^= (static_cast<size_t>(d->color.rgba()));
    return key;
}

bool BorderMaterial::isCompatibleWith(const Material& other) const {
    if (!Material::isCompatibleWith(other)) {
        return false;
    }
    
    const auto* otherBorder = dynamic_cast<const BorderMaterial*>(&other);
    if (!otherBorder) {
        return false;
    }
    
    // Совместимы если используют один и тот же цвет
    return d->color.rgba() == otherBorder->d->color.rgba();
}

void BorderMaterial::setupShaderPermutations() {
    // Без пермутаций для border материала
}

void BorderMaterial::bindSpecific() {
    // Специфичная привязка для border
}

void BorderMaterial::unbindSpecific() {
    // Специфичная отвязка для border
}
