#include "Content/ShadersModule/services/shaders_service.h"
#include "Content/ShadersModule/services/shader_features.h"
#include "Content/ShadersModule/data_providers/i_shaders_data_provider.h"
#include "Libs/Graphics/shaders/shader_program.h"
#include <map>

class ShadersService::Private {
public:
	Private(ShadersService* parent) : q(parent) {}

	ShadersService* q;
	IShadersDataProvider* shadersDataProvider = nullptr;
	std::map<QString, std::shared_ptr<ShaderProgram>> cache;

	QString buildingDefines(uint64_t features) {
		QString defines;

		auto appendDefine = [&](const char* name) {
			defines += "#define " + std::string(name) + "\n";
			};
		auto appendDefineValue = [&](const char* name, int value) {
			defines += "#define " + std::string(name) + " " + std::to_string(value) + "\n";
			};

		if (features & static_cast<uint64_t>(ShaderFeature::UseTexCoord)) {
			appendDefine("USE_TEXCOORD");
		}
		if (features & static_cast<uint64_t>(ShaderFeature::GridOverlay)) {
			appendDefine("PASS_GRID");
		}
		if (features & static_cast<uint64_t>(ShaderFeature::TileAnimation)) {
			appendDefine("TILE_ANIMATION");
		}
		if (features & static_cast<uint64_t>(ShaderFeature::AnimateUV)) {
			appendDefine("ANIMATE_UV");
		}

		return "#version 450 core\n" + defines;
	}
};

ShadersService::ShadersService(IShadersDataProvider* shadersDataProvider)
	: d(std::make_unique<Private>(this)) {
	d->shadersDataProvider = shadersDataProvider;
}

ShadersService::~ShadersService() = default;

std::shared_ptr<ShaderProgram> ShadersService::loadShader(const QString& name, uint32_t features) {

	const auto key = QString("%1_%2")
		.arg(name)
		.arg(QString::number(features));

	auto it = d->cache.find(key);
	if (it != d->cache.end()) {
		return it->second;
	}

	const auto defines = d->buildingDefines(features);

	const auto vertexSource = d->shadersDataProvider->loadShader(QString("shaders/%1.vert")
		.arg(name));
	if (!vertexSource.has_value()) {
		qWarning() << "ShadersService::createShader: failed to load vertex shader with name" << name;
		return std::shared_ptr<ShaderProgram>();
	}

	const auto fragmentSource = d->shadersDataProvider->loadShader(QString("shaders/%1.frag")
		.arg(name));
	if (!fragmentSource.has_value()) {
		qWarning() << "ShadersService::createShader: failed to load fragment shader  with name" << name;
		return std::shared_ptr<ShaderProgram>();
	}

	auto shader = std::make_shared<ShaderProgram>();
	auto createResult = shader->initialize(
		defines + vertexSource.value(),
		defines + fragmentSource.value());

	if (!createResult) {
		return std::shared_ptr<ShaderProgram>();
	}

	d->cache.emplace(key, shader);

	return shader;
}

void ShadersService::clear() {
	d->cache.clear();
}
