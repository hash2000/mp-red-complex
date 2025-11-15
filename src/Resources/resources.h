#pragma once
#include "Base/config.h"
#include "DataStream/data_stream/data_stream_container.h"
#include "Base/container_view.h"
#include <list>
#include <memory>
#include <optional>
#include <functional>

class Resources : public Configurable {
public:
	Resources();

	virtual ~Resources() = default;

public:
	void configure(const std::shared_ptr<Config>& config) override;

	void loadDatFile(const QString& fileName);

	void load();

	auto items() const {
    return make_unique_ptr_view(_resources);
	}

	auto getStream(const QString &container, const QString &path) const
		-> std::optional<std::reference_wrapper<DataStream>>;

private:
	QString _language;
	QDir _resources_path;
	std::list<std::unique_ptr<DataStreamContainer>> _resources;
};
