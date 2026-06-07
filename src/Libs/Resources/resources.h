#pragma once
#include "Libs/Resources/variables/variables_context.h"
#include "Libs/Base/config.h"
#include "Libs/DataStream/data_stream/data_stream_container.h"
#include "Libs/Base/container_view.h"
#include <list>
#include <memory>
#include <optional>

class DataWriteStream;

class Resources : public Configurable {
public:
	Resources();

	virtual ~Resources() = default;

public:
	void configure(const Config* config) override;
	void load();

	auto items() const {
    return make_deref_view(_resources);
	}

	auto getStream(const QString &container, const QString &path) const
		-> std::optional<std::shared_ptr<DataStream>>;

	auto getWriteStream(const QString& container, const QString& path) const
		-> std::optional<std::shared_ptr<DataWriteStream>>;

public:
	VariablesContext Variables;

private:
	void loadDatFile(const QString& fileName);
	void loadDatResources();
	void loadRawResources();

private:
	QDir _resources_path;
	QDir _resources_path_raw;
	std::list<std::unique_ptr<DataStreamContainer>> _resources;
};
