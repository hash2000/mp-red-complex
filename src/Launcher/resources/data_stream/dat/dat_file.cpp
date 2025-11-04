#include "Launcher/resources/data_stream/dat/dat_file.h"
#include "Launcher/resources/data_stream/dat/dat_file_exception.h"
#include "Launcher/resources/data_stream/data_stream_buffer.h"
#include <string.h>

void DatFile::loadFromFile(const QString &path)
{
	_stream.open(path.toStdString(), std::ios_base::binary);
	if (!_stream.is_open()) {
		throw DatFileException(path, "can't open");
	}

	_stream.seekg(0,std::ios::end);
	const auto size = static_cast<size_t>(_stream.tellg());

	uint32_t fileSize = 0;
	_stream.seekg(size - 4, std::ios::beg);
	_stream.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));
	if(size != fileSize) {
		throw DatFileException(path, "wrong file size");
	}

	uint32_t fileTreeSize = 0;
	_stream.seekg(size - 8, std::ios::beg);
	_stream.read(reinterpret_cast<char*>(&fileTreeSize), sizeof(fileTreeSize));

	uint32_t fileTotalCount = 0;
	_stream.seekg(size - fileTreeSize - 8, std::ios::beg);
	_stream.read(reinterpret_cast<char*>(&fileTotalCount), sizeof(fileTotalCount));

	for (uint32_t i = 0; i < fileTotalCount; i++) {
		auto buffer = loadStream();
		_streams.push_back(std::move(buffer));
	}
}

std::unique_ptr<DataStream> DatFile::loadStream() {

	std::string name;
	uint32_t nameSize;
	uint8_t compressed;
	uint32_t decompressedSize;
	uint32_t compressedSize;
	uint32_t dataOffset;

	_stream.read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
	name.resize(nameSize);
	_stream.read(&name.front(), nameSize);
	_stream.read(reinterpret_cast<char*>(&compressed), sizeof(compressed));
	_stream.read(reinterpret_cast<char*>(&decompressedSize), sizeof(decompressedSize));
	_stream.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
	_stream.read(reinterpret_cast<char*>(&dataOffset), sizeof(dataOffset));

	auto buffer = std::make_unique<DataStreamBuffer>();
	buffer->compressed(compressed == 1);
	buffer->decompressedSize(decompressedSize);
	buffer->compressedSize(compressedSize);
	buffer->dataOffset(dataOffset);
	buffer->name(QString::fromStdString(name));

	return std::move(buffer);
}
