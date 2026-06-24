#pragma once
#include <QString>
#include <memory>

class IShadersDataProvider;
class ShaderProgram;

class ShadersService {
public:
	ShadersService(IShadersDataProvider* shadersDataProvider);
	~ShadersService();

	std::shared_ptr<ShaderProgram> loadShader(const QString& name, uint32_t features);

	void clear();

private:
	class Private;
	std::unique_ptr<Private> d;
};
