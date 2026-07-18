#include "Libs/Resources/resources.h"
#include "Libs/DataStream/data_stream/dat/dat_file.h"
#include "Libs/DataStream/data_stream/raw/raw_directory.h"
#include "Libs/DataStream/data_write_stream/data_write_stream_file.h"
#include "Libs/Base/scoped_timer.h"

Resources::Resources() {
}

void Resources::configure(const Config* config) {
	_resources_path = QDir(config->resources_path);
	Variables.set("Resources.Path", _resources_path.absolutePath());
	Variables.set("Resources.Tools.Formatters.Path", QDir(config->resources_formatters_path).absolutePath());
	Variables.set("Resources.Tools.Formatter.Executable", QDir(config->resources_formatter_executable).absolutePath());
}

void Resources::load() {
	loadDatResources();
	loadRawResources();
}

void Resources::loadDatFile(const QString& fileName) {

	ScopedTimer watch(QString("Load %1 file.").arg(fileName));

	QFileInfo file(_resources_path.filePath(fileName));
	if(!file.exists()) {
		qWarning() << file.absoluteFilePath() << "not found";
	}

	for(const auto& res : _resources) {
		if (res->name() == fileName) {
			watch.error(QString("%1 allready loaded").arg(fileName));
			return;
		}
	}

	try {
		auto item = std::make_unique<DatFile>();
		item->name(fileName);
		item->loadFromFile(file.absoluteFilePath());
		_resources.push_back(std::move(item));
	}
	catch (std::exception &ex) {
		watch.error(ex.what());
		return;
	}
}

void Resources::loadDatResources() {
	qDebug() << "Resources::loadDatResources from" << _resources_path.absolutePath();
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
	qDebug() << "Resources::loadRawResources from" << _resources_path.absolutePath();
	const auto entries = _resources_path.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
	for(const auto &entry: entries) {
		const auto path = entry.absoluteFilePath();
		const auto fileName = entry.fileName();

		ScopedTimer watch("Load raw resources.");

		try {
			auto item = std::make_unique<RawDirectory>();
			item->name(fileName);
			item->loadFromPath(path);
			_resources.push_back(std::move(item));
		}
		catch (std::exception &ex) {
			watch.error(QString("%1 exception: %2")
				.arg(fileName)
				.arg(ex.what()));
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

auto Resources::getWriteStream(const QString& container, const QString& path) const
	-> std::optional<std::shared_ptr<DataWriteStream>> {
	const auto dataPath = QString("")
		.arg(_resources_path.absolutePath())
		.arg(container)
		.arg(path);
	auto stream = std::make_shared<DataWriteStreamFile>(dataPath);


	return stream;
}
