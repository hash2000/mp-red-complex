#include "Graphics/tiles/material.h"
#include <QOpenGLContext>

// ── Вершинный шейдер ──
static const char* s_vertexShaderSource = R"(
#version 330 core

// ── Атрибуты (всегда одинаковые) ──
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

// ── Uniform Buffer (std140 выравнивание!) ──
layout(std140, binding = 0) uniform ChunkData {
    mat4 projection;          // 64 байта
    mat4 view;                // 64 байта
    float zLevel;             // 4 байта
    float _pad1[3];           // 12 байт padding → всего 144 байта до flags
    
    uint flags;               // 4 байта (битовая маска)
    float _pad2[3];           // 12 байт padding
    
    // ── Динамические параметры (меняются каждый кадр) ──
    float time;               // Глобальное время для анимаций
    float tileAnimFrame;      // Кадр анимации тайлов (0..1)
    vec2 uvOffset;            // Смещение текстурных координат
    vec2 tileSize;            // Размер тайла в UV-пространстве (1/количество_тайлов)
    vec4 borderColor;         // Цвет обводки чанка
    vec4 wetParams;           // (сила искажения, сила блика, скорость, масштаб)
    vec4 fireParams;          // (интенсивность, скорость, цвет1, цвет2)
};

#ifdef USE_TEXCOORD
out vec2 vTexCoord;
#endif
#ifdef PASS_GRID
flat out int vTileX, vTileY;
#endif

void main() {
#ifdef USE_TEXCOORD
    vec2 uv = aTexCoord + uvOffset;
    
    // Тайловая анимация: сдвиг по кадрам
#   ifdef TILE_ANIMATION
        uv += floor(uv / tileSize) * tileAnimFrame;
#   endif
    
    // Анимация плавного сдвига (для воды/огня)
#   ifdef ANIMATE_UV
        uv += vec2(sin(time * 0.5 + aPosition.x) * 0.01, 
                   cos(time * 0.3 + aPosition.y) * 0.01);
#   endif
    
    vTexCoord = uv;
#endif

#ifdef PASS_GRID
    // Вычисляем координаты тайла для отрисовки сетки
    vec2 tileUV = aTexCoord / tileSize;
    vTileX = int(floor(tileUV.x));
    vTileY = int(floor(tileUV.y));
#endif

    // Позиция: всегда одинаковая
    gl_Position = projection * view * vec4(aPosition.x, zLevel, aPosition.y, 1.0);
}
)";

// ── Фрагментный шейдер ──
static const char* s_fragmentShaderSource = R"(
#version 330 core

#ifdef USE_TEXCOORD
in vec2 vTexCoord;
layout(binding = 0) uniform sampler2D uTexture;
layout(binding = 1) uniform sampler2D uNormalMap;  // опционально
#endif

#ifdef PASS_GRID
flat in int vTileX, vTileY;
layout(binding = 2) uniform sampler2D uGridPattern;  // 1x1 текстура с паттерном сетки
#endif

layout(std140, binding = 0) uniform ChunkData {
    mat4 projection;          // 64 байта
    mat4 view;                // 64 байта
    float zLevel;             // 4 байта
    float _pad1[3];           // 12 байт padding → всего 144 байта до flags
    
    uint flags;               // 4 байта (битовая маска)
    float _pad2[3];           // 12 байт padding
    
    // ── Динамические параметры (меняются каждый кадр) ──
    float time;               // Глобальное время для анимаций
    float tileAnimFrame;      // Кадр анимации тайлов (0..1)
    vec2 uvOffset;            // Смещение текстурных координат
    vec2 tileSize;            // Размер тайла в UV-пространстве (1/количество_тайлов)
    vec4 borderColor;         // Цвет обводки чанка
    vec4 wetParams;           // (сила искажения, сила блика, скорость, масштаб)
    vec4 fireParams;          // (интенсивность, скорость, цвет1, цвет2)
};

layout(location = 0) out vec4 FragColor;

// ── Утилиты ──
float random(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    // ── Режим: сплошной цвет (бордер чанка) ──
    if ((flags & 0x2) != 0u) {  // SOLID_COLOR
        FragColor = borderColor;
        return;
    }

    // ── Режим: текстура ──
    vec4 baseColor = vec4(1.0);
#ifdef USE_TEXCOORD
    baseColor = texture(uTexture, vTexCoord);
    
    // Альфа-отбрасывание
    if ((flags & 0x4) != 0u && baseColor.a < 0.1) {
        discard;
    }
#endif

    // ── Эффект: блики (Specular) ──
    if ((flags & 0x8) != 0u) {
#ifdef USE_TEXCOORD
        // Простая модель: ярче в центре тайла
        vec2 tileCenter = fract(vTexCoord / tileSize) - 0.5;
        float spec = pow(1.0 - length(tileCenter) * 1.4, 4.0);
        baseColor.rgb += vec3(1.0, 0.95, 0.8) * spec * 0.3;
#endif
    }

    // ── Эффект: мокрый пол (искажение + усиленные блики) ──
    if ((flags & 0x10) != 0u) {
#ifdef USE_TEXCOORD
        // Искажение UV на основе шума
        float noise = random(vTexCoord * wetParams.w + time * wetParams.z);
        vec2 distortion = vec2(sin(noise * 6.28), cos(noise * 6.28)) * wetParams.x * 0.02;
        vec4 distorted = texture(uTexture, vTexCoord + distortion);
        
        // Усиленные блики
        float wetSpec = pow(1.0 - abs(dot(normalize(vec3(distortion.rgb - 0.5)), vec3(0,0,1))), 8.0);
        baseColor = mix(baseColor, distorted, 0.7);
        baseColor.rgb += vec3(0.8, 0.9, 1.0) * wetSpec * wetParams.y;
#endif
    }

    // ── Эффект: огонь (анимация + цветовая интерполяция) ──
    if ((flags & 0x20) != 0u) {
#ifdef USE_TEXCOORD
        float flicker = sin(time * fireParams.y + vTexCoord.x * 10.0) * 0.5 + 0.5;
        vec3 fireColor = mix(vec3(fireParams.z, fireParams.w, 0.0), vec3(fireParams.x, fireParams.y, 0.0), flicker);
        baseColor.rgb = mix(baseColor.rgb, fireColor, fireParams.x * flicker);
        // Добавляем "дрожание" альфа-канала
        baseColor.a *= 0.8 + flicker * 0.4;
#endif
    }

    // ── Оверлей: сетка тайлов ──
    if ((flags & 0x40) != 0u) {
#ifdef PASS_GRID
        // Рисуем сетку: линии на границах тайлов
        vec2 gridUV = fract(vTexCoord / tileSize);
        float line = smoothstep(0.0, 0.02, min(gridUV.x, gridUV.y));
        line *= smoothstep(1.0, 0.98, max(gridUV.x, gridUV.y));
        vec3 gridColor = vec3(0.0, 0.0, 0.0) * (1.0 - line);
        baseColor.rgb = mix(baseColor.rgb, gridColor, 0.5);
#endif
    }

    // ── Оверлей: обводка чанка (отдельный pass, рисуется поверх) ──
    if ((flags & 0x80) != 0u) {
        baseColor = borderColor;
    }

    FragColor = baseColor;
}
)";

class SharedShaderProgram::Private {
public:
    Private() = default;
    
    QMatrix4x4 projection;
    QMatrix4x4 view;
    float currentTime = 0.0f;
};

SharedShaderProgram& SharedShaderProgram::instance() {
    static SharedShaderProgram instance;
    return instance;
}

SharedShaderProgram::~SharedShaderProgram() {
    if (m_ubo.isCreated()) {
        m_ubo.destroy();
    }
}

bool SharedShaderProgram::initialize() {
    if (m_initialized) {
        return true;
    }
    
    if (!m_shader.addShaderFromSourceCode(QOpenGLShader::Vertex, s_vertexShaderSource)) {
        qWarning() << "SharedShaderProgram: failed to compile vertex shader:" << m_shader.log();
        return false;
    }
    
    if (!m_shader.addShaderFromSourceCode(QOpenGLShader::Fragment, s_fragmentShaderSource)) {
        qWarning() << "SharedShaderProgram: failed to compile fragment shader:" << m_shader.log();
        return false;
    }
    
    if (!m_shader.link()) {
        qWarning() << "SharedShaderProgram: failed to link shader program:" << m_shader.log();
        return false;
    }
    
    // Создаем UBO
    m_ubo.create();
    m_ubo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
    
    // Размер UBO согласно std140:
    // projection: 64, view: 64, zLevel+padding: 16, flags+padding: 16
    // time: 4, tileAnimFrame: 4, uvOffset: 8, tileSize: 8, borderColor: 16, wetParams: 16, fireParams: 16
    // Итого: 64+64+16+16+4+4+8+8+16+16+16 = 232 байта, выравниваем до 256
    constexpr int kUBOSize = 256;
    m_ubo.allocate(kUBOSize);
    
    m_initialized = true;
    qInfo() << "SharedShaderProgram initialized with single shader for all materials";
    return true;
}

void SharedShaderProgram::bind() {
    m_shader.bind();
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_ubo.bufferId());
}

void SharedShaderProgram::release() {
    m_shader.release();
}

void SharedShaderProgram::setMatrices(const QMatrix4x4& projection, const QMatrix4x4& view) {
    d->projection = projection;
    d->view = view;
}

void SharedShaderProgram::setTime(float time) {
    d->currentTime = time;
}

void SharedShaderProgram::updateParams(const MaterialParams& params) {
    if (!m_initialized) return;
    
    // Заполняем данные UBO
    struct alignas(16) UBOData {
        QMatrix4x4 projection;
        QMatrix4x4 view;
        float zLevel;
        float _pad1[3];
        uint flags;
        float _pad2[3];
        float time;
        float tileAnimFrame;
        QVector2D uvOffset;
        QVector2D tileSize;
        QVector4D borderColor;
        QVector4D wetParams;
        QVector4D fireParams;
    };
    
    UBOData data;
    data.projection = d->projection;
    data.view = d->view;
    data.zLevel = params.zLevel;
    data._pad1[0] = data._pad1[1] = data._pad1[2] = 0.0f;
    data.flags = params.flags;
    data._pad2[0] = data._pad2[1] = data._pad2[2] = 0.0f;
    data.time = params.time > 0 ? params.time : d->currentTime;
    data.tileAnimFrame = params.tileAnimFrame;
    data.uvOffset = params.uvOffset;
    data.tileSize = params.tileSize;
    data.borderColor = QVector4D(params.borderColor.redF(), params.borderColor.greenF(), 
                                  params.borderColor.blueF(), params.borderColor.alphaF());
    data.wetParams = params.wetParams;
    data.fireParams = params.fireParams;
    
    m_ubo.write(0, &data, sizeof(UBOData));
}

void SharedShaderProgram::bindTexture(int unit, QOpenGLTexture* texture) {
    if (texture && texture->isCreated()) {
        texture->bind(unit);
    }
}

// ─────────────────────────────────────────────────────────────
class Material::Private {
public:
    Private(Material* parent) : q(parent) {}
    
    Material* q;
};

Material::Material() 
    : d(std::make_unique<Private>(this)) {
}

Material::~Material() = default;

bool Material::initialize() {
    // Инициализируем общую шейдерную программу
    auto& sharedShader = SharedShaderProgram::instance();
    if (!sharedShader.isInitialized()) {
        if (!sharedShader.initialize()) {
            qWarning() << "Material: failed to initialize shared shader program";
            return false;
        }
    }
    
    setupParams();
    return true;
}

void Material::bind() {
    auto& sharedShader = SharedShaderProgram::instance();
    sharedShader.bind();
    
    // Обновляем параметры в UBO
    sharedShader.updateParams(m_params);
    
    bindSpecific();
}

void Material::unbind() {
    unbindSpecific();
    SharedShaderProgram::instance().release();
}

void Material::setZLevel(float value) {
    m_params.zLevel = value;
}

size_t Material::sortKey() const {
    // Базовый ключ сортировки - тип материала и флаги
    size_t key = static_cast<size_t>(m_type);
    key ^= static_cast<size_t>(m_params.flags);
    return key;
}

bool Material::isCompatibleWith(const Material& other) const {
    // Совместимы если одинаковый тип и флаги
    return m_type == other.m_type && m_params.flags == other.m_params.flags;
}

void Material::setFlag(uint flag, bool enabled) {
    if (enabled) {
        m_params.flags |= flag;
    } else {
        m_params.flags &= ~flag;
    }
}

void Material::setupParams() {
    // Базовая реализация - пустая
}

void Material::bindSpecific() {
    // Базовая реализация - пустая
}

void Material::unbindSpecific() {
    // Базовая реализация - пустая
}
