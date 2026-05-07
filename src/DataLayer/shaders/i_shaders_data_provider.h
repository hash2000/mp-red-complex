#pragma once
#include <QString>
#include <optional>

class IShadersDataProvider {
public:
	virtual std::optional<QString> LoadShader(const QString& path) = 0;
};
