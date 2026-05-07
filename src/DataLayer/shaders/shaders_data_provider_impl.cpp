#include "DataLayer/shaders/shaders_data_provider_impl.h"
#include "DataStream/format/text/data_reader.h"
#include "Resources/resources.h"

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

std::optional<QString> ShadersDataProviderLocalImpl::LoadShader(const QString& path) {
	QString shader;
	Format::Text::DataReader reader(d->resources, "shaders", path);
	if (!reader.read(shader)) {
		return std::nullopt;
	}

	return shader;
}
