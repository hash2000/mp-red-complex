#include "DataStream/data_stream/raw/raw_directory.h"
#include "DataStream/data_stream/data_stream_file.h"
#include <QDirIterator>
#include <QFileInfo>
#include <fstream>

ContainerType RawDirectory::type() const {
	return ContainerType::Directory;
}

void RawDirectory::loadFromPath(const QString &path) {
	_path = QDir(name()).filePath(path);
}

std::map<QString, std::shared_ptr<DataStream>> RawDirectory::items() const {
	std::map<QString, std::shared_ptr<DataStream>> result;

	QDirIterator iterator(_path.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
	while (iterator.hasNext()) {
		const auto fullPath = iterator.next();
		const auto relative = _path
			.relativeFilePath(fullPath)
			.toLower();

		try {
			auto stream = std::make_shared<std::ifstream>();
			stream->open(fullPath.toStdString(), std::ios_base::binary);
			stream->seekg(0, std::ios::end);
			const auto size = static_cast<size_t>(stream->tellg());
			auto buffer = std::make_unique<DataStreamFile>(stream);
			buffer->compressed(false);
			buffer->compressedSize(size);
			buffer->decompressedSize(size);
			buffer->dataOffset(0);
			buffer->name(relative);
			buffer->containerName(name());
			buffer->type(type());
			result.emplace(relative, std::move(buffer));
		}
		catch (std::exception& ex) {
			qWarning() << "Load" << fullPath << "exception:" << ex.what();
		}
	}

	return result;
}

std::optional<std::shared_ptr<DataStream>> RawDirectory::find(const QString &name) const {
	const auto streams = items();
	const auto it = streams.find(name);
	if (it == streams.end()) {
		return std::nullopt;
	}
	return it->second;
}

void RawDirectory::add(std::shared_ptr<DataStream> stream) {
	Q_UNUSED(stream);
	qWarning() << "RawDirectory::add() is not supported - in-memory storage is not available";
}

void RawDirectory::remove(const QString &name) {
	Q_UNUSED(name);
	qWarning() << "RawDirectory::remove() is not supported - in-memory storage is not available";
}

void RawDirectory::clear() {
	qWarning() << "RawDirectory::clear() is not supported - in-memory storage is not available";
}
