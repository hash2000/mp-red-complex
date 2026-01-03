#pragma once
#include "DataStream/data_write_stream.h"
#include <QFile>
#include <QSaveFile>
#include <stdexcept>
#include <system_error>


class DataWriteStreamFile : public DataWriteStream {
public:
	explicit DataWriteStreamFile(QFile* file, bool takeOwnership = false);
	explicit DataWriteStreamFile(const QString& filePath);
	explicit DataWriteStreamFile(std::unique_ptr<QSaveFile> saveFile);

	~DataWriteStreamFile() override;

	void writeRaw(const void* src, size_t size) override;
	size_t size() const override;

	void position(size_t pos) override;
	size_t position() const override;

	void skip(size_t n) override;
	bool isWritable() const;
	bool commit();

private:
	QIODevice* _file = nullptr;
	std::unique_ptr<QSaveFile> _saveFile;
	bool _ownsFile = false;
};
