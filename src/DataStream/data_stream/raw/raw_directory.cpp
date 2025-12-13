#include "DataStream/data_stream/raw/raw_directory.h"
#include "DataStream/data_stream/data_stream_file.h"
#include <QDirIterator>
#include <QFileInfo>
#include <fstream>

void RawDirectory::loadFromPath(const QString &path) {
	_path = QDir(path);
	QDirIterator iterator(_path.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
	while (iterator.hasNext()) {
		const auto fullPath = iterator.next();
		const auto relative = _path
			.relativeFilePath(fullPath)
			.replace('/', '\\');

		auto stream = std::make_shared<std::ifstream>();
		stream->open(fullPath.toStdString(), std::ios_base::binary);
		stream->seekg(0,std::ios::end);
		const auto size = static_cast<size_t>(stream->tellg());
		auto buffer = std::make_unique<DataStreamFile>(stream);
		buffer->compressed(false);
		buffer->compressedSize(size);
		buffer->decompressedSize(size);
		buffer->dataOffset(0);
		buffer->name(relative);
		add(std::move(buffer));
	}
}
