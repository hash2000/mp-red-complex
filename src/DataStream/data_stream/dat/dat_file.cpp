#include "DataStream/data_stream/dat/dat_file.h"
#include "DataStream/data_stream/dat/dat_file_exception.h"
#include "DataStream/data_stream/data_stream_file.h"
#include <string.h>


ContainerType DatFile::type() const {
	return ContainerType::Repository_v1;
}

void DatFile::loadFromFile(const QString &path)
{
	auto stream = std::make_shared<std::ifstream>();
	stream->open(path.toStdString(), std::ios_base::binary);
	if (!stream->is_open()) {
		throw DatFileException(path, "can't open");
	}

	stream->seekg(0,std::ios::end);
	const auto size = static_cast<size_t>(stream->tellg());

	uint32_t fileSize = 0;
	stream->seekg(size - 4, std::ios::beg);
	stream->read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));
	if(size != fileSize) {
		throw DatFileException(path, "wrong file size");
	}

	uint32_t fileTreeSize = 0;
	stream->seekg(size - 8, std::ios::beg);
	stream->read(reinterpret_cast<char*>(&fileTreeSize), sizeof(fileTreeSize));

	uint32_t fileTotalCount = 0;
	stream->seekg(size - fileTreeSize - 8, std::ios::beg);
	stream->read(reinterpret_cast<char*>(&fileTotalCount), sizeof(fileTotalCount));

	for (uint32_t i = 0; i < fileTotalCount; i++) {
		auto buffer = loadStream(stream);
		buffer->containerName(name());
		buffer->type(type());
		add(std::move(buffer));
	}
}

std::unique_ptr<DataStream> DatFile::loadStream(std::shared_ptr<std::ifstream> &stream) {

	std::string name;
	uint32_t nameSize;
	uint8_t compressed;
	uint32_t decompressedSize;
	uint32_t compressedSize;
	uint32_t dataOffset;

	stream->read(reinterpret_cast<char*>(&nameSize), sizeof(nameSize));
	name.resize(nameSize);
	stream->read(&name.front(), nameSize);
	stream->read(reinterpret_cast<char*>(&compressed), sizeof(compressed));
	stream->read(reinterpret_cast<char*>(&decompressedSize), sizeof(decompressedSize));
	stream->read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
	stream->read(reinterpret_cast<char*>(&dataOffset), sizeof(dataOffset));

	auto buffer = std::make_unique<DataStreamFile>(stream);
	buffer->compressed(compressed == 1);
	buffer->decompressedSize(decompressedSize);
	buffer->compressedSize(compressedSize);
	buffer->dataOffset(dataOffset);
	buffer->name(QString::fromStdString(name)
		.replace("\\", "/")
		.toLower());

	return std::move(buffer);
}
