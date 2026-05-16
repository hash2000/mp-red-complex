#pragma once
#include <QString>
#include <optional>

class IShadersDataProvider {
public:
	virtual std::optional<QString> loadShader(const QString& path) = 0;
};
