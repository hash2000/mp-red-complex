#pragma once
#include "Libs/Resources/variables/variables_context.h"
#include "Libs/Resources/directories/directories_context.h"
#include "Libs/Base/config.h"
#include "Libs/DataStream/data_stream/data_stream_container.h"
#include "Libs/Base/container_view.h"

#include <QDir>

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
	bool load();
	bool loadProfile(const QString& userHash = QString());
	std::optional<QDir> createProfilePath(const QString& userHash);

	QString defaultUseerName() const;

	auto items() const {
    return make_deref_view(_resources);
	}

	auto getStream(const QString &container, const QString &path) const
		-> std::optional<std::shared_ptr<DataStream>>;

	auto getWriteStream(const QString& container, const QString& path) const
		-> std::optional<std::shared_ptr<DataWriteStream>>;

public:
	VariablesContext Variables;
	DirectoriesContext Directories;

private:
	void loadDatFile(const QString& fileName);
	void loadDatResources();
	void loadRawResources();
	bool loadDefaults();

private:
	VariablesContext _defaultsVariables;
	DirectoriesContext _defaultsDirectories;
	std::list<std::unique_ptr<DataStreamContainer>> _resources;
};
