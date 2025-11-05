#pragma once
#include "Launcher/config.h"
#include "Launcher/resources/data_stream/data_stream_container.h"
#include <list>
#include <memory>

class Resources : public Configurable {
public:
	Resources();

	virtual ~Resources() = default;

public:
	void configure(const std::shared_ptr<Config>& config) override;

	void loadDatFile(const QString& path);

private:
	QString _language;
	QDir _resources_path;
	std::list<std::unique_ptr<DataStreamContainer>> _resources;
};
