#include "DataLayer/shaders/shaders_data_provider_impl.h"
#include "Libs/DataStream/format/text/data_reader.h"
#include "Libs/Resources/resources.h"

class ShadersDataProviderLocalImpl::Private {
public:
	Private(ShadersDataProviderLocalImpl* parent) : q(parent) {}
	ShadersDataProviderLocalImpl* q;
	Resources* resources = nullptr;
};

ShadersDataProviderLocalImpl::ShadersDataProviderLocalImpl(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

ShadersDataProviderLocalImpl::~ShadersDataProviderLocalImpl() = default;

std::optional<QString> ShadersDataProviderLocalImpl::loadShader(const QString& path) {
	QString shader;
	Format::Text::DataReader reader(d->resources, "assets", path);
	if (!reader.read(shader)) {
		return std::nullopt;
	}

	return shader;
}
