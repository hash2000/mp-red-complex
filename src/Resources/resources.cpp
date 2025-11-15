#include "Resources/resources.h"
#include "Resources/resources/data_stream/dat/dat_file.h"
#include "Resources/resources/data_stream/dat/dat_file_exception.h"

Resources::Resources() {
}

void Resources::configure(const std::shared_ptr<Config> &config) {
	_resources_path = QDir(config->resources_path);
	_language = config->resources_language;
}

void Resources::loadDatFile(const QString& fileName) {
	QFileInfo file(_resources_path.filePath(fileName));
	if(!file.exists()) {
		throw DatFileException(file.absoluteFilePath(), "not found");
	}

	for(const auto& res : _resources) {
		if (res->name() == fileName) {
			qWarning() << fileName << " allready loaded";
			return;
		}
	}

	auto item = std::make_unique<DatFile>();
	item->loadFromFile(file.absoluteFilePath());
	item->name(fileName);

	_resources.push_back(std::move(item));
}

void Resources::load() {
	QDir dir(_resources_path);
	const auto entries = dir.entryInfoList(QDir::Files);

	for(const auto &entry: entries) {
		const auto fileName = entry.fileName();
		const auto suffix = entry.suffix().toLower();

		if (suffix == "dat") {
			loadDatFile(fileName);
		}
	}
}

auto Resources::getStream(const QString &container, const QString &path) const
	-> std::optional<std::reference_wrapper<DataStream>> {
	for (const auto& res : _resources) {
		if (res->name() == container) {
			return res->find(path);
		}
	}

	return std::nullopt;
}
