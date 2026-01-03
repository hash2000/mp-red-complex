#include "DataStream/data_write_stream/data_write_stream_file.h"

DataWriteStreamFile::DataWriteStreamFile(QFile* file, bool takeOwnership)
	: _file(file)
	, _ownsFile(takeOwnership)
{
	if (!_file || !_file->isOpen()) {
		throw std::invalid_argument("DataWriteStreamFile: file must be open");
	}

	if ((_file->openMode() & QIODevice::WriteOnly) == 0) {
		throw std::invalid_argument("DataWriteStreamFile: file must be opened in WriteOnly mode");
	}
}

DataWriteStreamFile::DataWriteStreamFile(const QString& filePath)
	: _file(new QFile(filePath))
	, _ownsFile(true)
{
	if (!_file->open(QIODevice::WriteOnly)) {
		const QString err = _file->errorString();
		delete _file;
		throw std::runtime_error("Failed to open file for writing: " + err.toStdString());
	}
}

DataWriteStreamFile::DataWriteStreamFile(std::unique_ptr<QSaveFile> saveFile)
	: _saveFile(std::move(saveFile))
	, _file(_saveFile.get())
	, _ownsFile(true)
{
	if (!_file || !_file->isOpen()) {
		throw std::runtime_error("QSaveFile failed to open");
	}
}

DataWriteStreamFile::~DataWriteStreamFile() {
	if (_ownsFile) {
		if (_saveFile) {
			if (!_saveFile->commit()) {
				// commit failed — QSaveFile auto-rollback
				// Можно логгировать, но exception в деструкторе — плохо
			}
		}
		else {
			delete _file;
		}
	}
}

void DataWriteStreamFile::writeRaw(const void* src, size_t size) {
	if (size == 0) {
		return;
	}

	const qint64 written = _file->write(static_cast<const char*>(src), static_cast<qint64>(size));
	if (written != static_cast<qint64>(size)) {
		throw std::runtime_error(
			"DataWriteStreamFile::writeRaw failed: " + _file->errorString().toStdString()
		);
	}
}

size_t DataWriteStreamFile::size() const {
	return static_cast<size_t>(_file->size());
}

void DataWriteStreamFile::position(size_t pos) {
	if (!_file->seek(static_cast<qint64>(pos))) {
		throw std::runtime_error(
			"DataWriteStreamFile::position(" + std::to_string(pos) +
			") failed: " + _file->errorString().toStdString()
		);
	}
}

size_t DataWriteStreamFile::position() const {
	return static_cast<size_t>(_file->pos());
}

void DataWriteStreamFile::skip(size_t n) {
	position(position() + n);
}

bool DataWriteStreamFile::isWritable() const {
	return _file && _file->isWritable();
}

bool DataWriteStreamFile::commit() {
	if (_saveFile) {
		return _saveFile->commit();
	}
	return true;
}
