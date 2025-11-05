#include "Launcher/resources.h"
#include "Launcher/resources/data_stream/dat/dat_file.h"
#include "Launcher/resources/data_stream/dat/dat_file_exception.h"

Resources::Resources() {
}

void Resources::configure(const std::shared_ptr<Config> &config) {
	_resources_path = QDir(config->resources_path);
	_language = config->resources_language;
}

void Resources::loadDatFile(const QString& path) {
	QFileInfo file(_resources_path.filePath(path));
	if(!file.exists()) {
		throw DatFileException(file.absoluteFilePath(), "not found");
	}

	for(const auto& res : _resources) {
		if (res->name() == path) {
			qWarning() << path << " allready loaded";
			return;
		}
	}

	auto item = std::make_unique<DatFile>();
	item->loadFromFile(file.absoluteFilePath());
	item->name(path);

	_resources.push_back(std::move(item));
}

