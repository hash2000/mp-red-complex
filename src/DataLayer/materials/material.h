#pragma once
#include <QVector4D>
#include <list>

class Material {
public:
	QVector4D baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
	float metallic = 0.0f; 
	float roughness = 0.5f;
	float normalScale = 1.0f;
	float occlusionStrength = 1.0f;
	QVector4D emissive{ 0.0f, 0.0f, 0.0f, 0.0f }; 
	float alphaCutoff = 0.5f;
	std::list<QString> flags;
};
