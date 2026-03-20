#pragma once
#include "DataStream/data_stream.h"
#include "DataStream/data_stream/data_stream_container.h"
#include <memory>
#include <fstream>
#include <QString>
#include <map>

class DatFile : public DataStreamContainer {
public:
	void loadFromFile(const QString &path);
	ContainerType type() const override;

	std::map<QString, std::shared_ptr<DataStream>> items() const override;
	std::optional<std::shared_ptr<DataStream>> find(const QString &name) const override;

protected:
	void add(std::shared_ptr<DataStream> stream) override;
	void remove(const QString &name) override;
	void clear() override;

private:
	std::unique_ptr<DataStream> loadStream(std::shared_ptr<std::ifstream> &stream);

private:
	std::map<QString, std::shared_ptr<DataStream>> _streams;
};
