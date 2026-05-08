#pragma once
#include <memory>
#include <vector>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QHash>
#include <QString>
#include <QOpenGLBuffer>

/// Битовые флаги для пермутаций шейдера
namespace ShaderFlags {
    constexpr uint None           = 0x0;
    constexpr uint UseTexCoord    = 0x1;      // Использовать текстурные координаты
    constexpr uint SolidColor     = 0x2;      // Сплошной цвет (без текстуры)
    constexpr uint AlphaTest      = 0x4;      // Альфа-отбрасывание
    constexpr uint Specular       = 0x8;      // Блик
    constexpr uint Wet            = 0x10;     // Мокрый пол
    constexpr uint Fire           = 0x20;     // Огонь
    constexpr uint GridOverlay    = 0x40;     // Сетка поверх
    constexpr uint ChunkBorder    = 0x80;     // Обводка чанка
    constexpr uint TileAnimation  = 0x100;    // Анимация тайлов
    constexpr uint AnimateUV      = 0x200;    // Плавное смещение UV
}

/// Параметры материала для настройки шейдера
struct MaterialParams {
    float time = 0.0f;              // Глобальное время
    float tileAnimFrame = 0.0f;     // Кадр анимации тайлов
    QVector2D uvOffset{0.0f, 0.0f}; // Смещение UV
    QVector2D tileSize{1.0f, 1.0f}; // Размер тайла в UV
    QColor borderColor{1.0f, 1.0f, 1.0f, 1.0f};
    QVector4D wetParams{0.0f, 0.0f, 0.0f, 1.0f};    // (искажение, блик, скорость, масштаб)
    QVector4D fireParams{0.0f, 0.0f, 0.0f, 0.0f};   // (интенсивность, скорость, r, g) - b,a для цвета
    float zLevel = 0.0f;
    uint flags = ShaderFlags::None;
};

/// Единая шейдерная программа для всех материалов
class SharedShaderProgram {
public:
    static SharedShaderProgram& instance();
    
    bool initialize();
    void bind();
    void release();
    
    /// Установить матрицы проекции и вида
    void setMatrices(const QMatrix4x4& projection, const QMatrix4x4& view);
    
    /// Установить глобальное время
    void setTime(float time);
    
    /// Обновить параметры UBO
    void updateParams(const MaterialParams& params);
    
    /// Привязать текстуру на указанный юнит
    void bindTexture(int unit, QOpenGLTexture* texture);
    
    QOpenGLShaderProgram* program() { return &m_shader; }
    bool isInitialized() const { return m_initialized; }
    
private:
    SharedShaderProgram() = default;
    ~SharedShaderProgram();
    
    class Private;
    std::unique_ptr<Private> d;
    
    QOpenGLShaderProgram m_shader;
    QOpenGLBuffer m_ubo;
    bool m_initialized = false;
};

/// Материал - описывает состояние рендеринга (параметры, текстуры, флаги)
class Material {
public:
    Material();
    virtual ~Material();

    /// Инициализация материала (привязка к общей шейдерной программе)
    virtual bool initialize();

    /// Привязать материал перед рендерингом
    virtual void bind();

    /// Отвязать материал после рендеринга
    virtual void unbind();

    /// Установить zLevel
    void setZLevel(float value);
    float zLevel() const { return m_params.zLevel; }

    /// Получить ключ сортировки (для группировки одинаковых материалов)
    virtual size_t sortKey() const;

    /// Проверка на совместимость с другим материалом (можно ли батчить)
    virtual bool isCompatibleWith(const Material& other) const;

    /// Тип материала для пермутаций шейдера
    enum class Type {
        Tileset,        // Базовый тайлсет с текстурой
        Border,         // Цветная рамка (без текстуры)
        AnimatedTile,   // Анимированный тайл
        Effect,         // Эффекты (огонь, блики и т.д.)
        Custom          // Пользовательский материал
    };

    Type type() const { return m_type; }
    QString name() const { return m_name; }
    
    /// Получить параметры материала
    const MaterialParams& params() const { return m_params; }
    MaterialParams& params() { return m_params; }
    
    /// Установить флаг шейдера
    void setFlag(uint flag, bool enabled);
    bool hasFlag(uint flag) const { return (m_params.flags & flag) != 0; }

protected:
    Type m_type = Type::Custom;
    QString m_name;
    MaterialParams m_params;
    
    /// Настроить специфичные параметры материала
    virtual void setupParams();
    
    /// Специфичная привязка (текстуры и т.д.)
    virtual void bindSpecific();
    virtual void unbindSpecific();
};
