#pragma once
#include <QString>
#include <QPixmap>
#include <optional>

class ITexturesDataProvider {
public:
	virtual ~ITexturesDataProvider() = default;

	// Загрузка текстуры по пути (относительно assets/icons)
	virtual std::optional<QPixmap> loadTexture(const QString& path) const = 0;
};
