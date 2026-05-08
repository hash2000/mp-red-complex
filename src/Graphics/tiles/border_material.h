#pragma once
#include "Graphics/tiles/material.h"
#include <QColor>
#include <memory>

/// Материал для рендеринга отладочных рамок (без текстуры, только цвет)
class BorderMaterial : public Material {
public:
    BorderMaterial();
    ~BorderMaterial();

    /// Установить цвет рамки
    void setColor(const QColor& color);
    QColor color() const;

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
