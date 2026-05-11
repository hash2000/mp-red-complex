#pragma once
#include <QVector2D>
#include <QMatrix4x4>
#include <QColor>
#include <memory>

/// Битовые флаги для пермутаций шейдера
namespace ShaderFlags {
	constexpr uint None = 0x0;
	constexpr uint UseTexCoord = 0x1;      // Использовать текстурные координаты
	constexpr uint SolidColor = 0x2;      // Сплошной цвет (без текстуры)
	constexpr uint AlphaTest = 0x4;      // Альфа-отбрасывание
	constexpr uint Specular = 0x8;      // Блик
	constexpr uint Wet = 0x10;     // Мокрый пол
	constexpr uint Fire = 0x20;     // Огонь
	constexpr uint GridOverlay = 0x40;     // Сетка поверх
	constexpr uint ChunkBorder = 0x80;     // Обводка чанка
	constexpr uint TileAnimation = 0x100;    // Анимация тайлов
	constexpr uint AnimateUV = 0x200;    // Плавное смещение UV
}

/// Параметры материала для настройки шейдера
struct MaterialParams {
	float time = 0.0f;              // Глобальное время
	float tileAnimFrame = 0.0f;     // Кадр анимации тайлов
	QVector2D uvOffset{ 0.0f, 0.0f }; // Смещение UV
	QVector2D tileSize{ 1.0f, 1.0f }; // Размер тайла в UV
	QColor borderColor{ 255, 0, 0, 255 };
	QVector4D wetParams{ 0.0f, 0.0f, 0.0f, 1.0f };    // (искажение, блик, скорость, масштаб)
	QVector4D fireParams{ 0.0f, 0.0f, 0.0f, 0.0f };   // (интенсивность, скорость, r, g) - b,a для цвета
	float zLevel = 0.0f;
	uint flags = ShaderFlags::None;
};
