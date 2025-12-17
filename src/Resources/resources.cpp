#include "Resources/resources.h"
#include "DataStream/data_stream/dat/dat_file.h"
#include "DataStream/data_stream/dat/dat_file_exception.h"
#include "DataStream/data_stream/raw/raw_directory.h"


Resources::Resources() {
}

void Resources::configure(const std::shared_ptr<Config> &config) {
	_resources_path = QDir(config->resources_path);
	_resources_path_raw = QDir(config->resources_path_raw);
	_language = config->resources_language;
}

void Resources::loadDatFile(const QString& fileName) {
	QFileInfo file(_resources_path.filePath(fileName));
	if(!file.exists()) {
		throw DatFileException(file.absoluteFilePath(), "not found");
	}

	for(const auto& res : _resources) {
		if (res->name() == fileName) {
			qWarning() << fileName << "allready loaded";
			return;
		}
	}

	try {
		auto item = std::make_unique<DatFile>();
		item->name(QString("%1 *").arg(fileName));
		item->loadFromFile(file.absoluteFilePath());
		_resources.push_back(std::move(item));
	}
	catch (std::exception &ex) {
		qWarning() << fileName << "exception:" << ex.what();
		return;
	}
}

void Resources::load() {
	loadDatResources();
	loadRawResources();
}

void Resources::loadDatResources() {
	const auto entries = _resources_path.entryInfoList(QDir::Files);

	for(const auto &entry: entries) {
		const auto fileName = entry.fileName();
		const auto suffix = entry.suffix().toLower();

		if (suffix == "dat") {
			loadDatFile(fileName);
		}
	}
}

void Resources::loadRawResources() {
	const auto entries = _resources_path_raw.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	for(const auto &entry: entries) {
		const auto path = entry.absoluteFilePath();
		const auto fileName = entry.fileName();

		try {
			auto item = std::make_unique<RawDirectory>();
			item->name(QString("%1 [_]").arg(fileName));
			item->loadFromPath(path);
			_resources.push_back(std::move(item));
		}
		catch (std::exception &ex) {
			qWarning() << fileName << "exception:" << ex.what();
			return;
		}
	}
}

auto Resources::getStream(const QString &container, const QString &path) const
	-> std::optional<std::shared_ptr<DataStream>> {
	for (const auto& res : _resources) {
		if (res->name() == container) {
			return res->find(path);
		}
	}

	return std::nullopt;
}
