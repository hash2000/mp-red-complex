#include "ApplicationLayer/shaders/shaders_service.h"
#include "DataLayer/shaders/i_shaders_data_provider.h"
#include "Graphics/shaders/shader_program.h"

class ShadersService::Private {
public:
	Private(ShadersService* parent) : q(parent) {}

	ShadersService* q;
	IShadersDataProvider* shadersDataProvider = nullptr;
	std::map<QString, std::shared_ptr<SharedShaderProgram>> cache;
};

ShadersService::ShadersService(IShadersDataProvider* shadersDataProvider)
	: d(std::make_unique<Private>(this)) {
	d->shadersDataProvider = shadersDataProvider;
}

ShadersService::~ShadersService() = default;

std::shared_ptr<SharedShaderProgram> ShadersService::loadShader(const QString& name) {

	auto it = d->cache.find(name);
	if (it != d->cache.end()) {
		return it->second;
	}

	const auto vertexSource = d->shadersDataProvider->loadShader(QString("%1.vert")
		.arg(name));
	if (!vertexSource.has_value()) {
		qWarning() << "ShadersService::createShader: failed to load vertex shader with name" << name;
		return std::shared_ptr<SharedShaderProgram>();
	}

	const auto fragmentSource = d->shadersDataProvider->loadShader(QString("%1.frag")
		.arg(name));
	if (!fragmentSource.has_value()) {
		qWarning() << "ShadersService::createShader: failed to load fragment shader  with name" << name;
		return std::shared_ptr<SharedShaderProgram>();
	}

	auto shader = std::make_shared<SharedShaderProgram>();
	auto createResult = shader->initialize(vertexSource.value(), fragmentSource.value());
	if (!createResult) {
		return std::shared_ptr<SharedShaderProgram>();
	}

	d->cache.emplace(name, shader);

	return shader;
}

void ShadersService::clear() {
	d->cache.clear();
}
