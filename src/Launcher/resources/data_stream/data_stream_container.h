#pragma once
#include "Launcher/resources/data_stream.h"
#include <QString>
#include <map>
#include <memory>

class DataStreamContainer {
public:
	virtual ~DataStreamContainer() = default;

	const QString name() const;

	void name(const QString &name);

protected:
	void add(const QString &name, std::unique_ptr<DataStream> &&stream);

	void remove(const QString &name);

	void clear();

private:
	std::map<QString, std::unique_ptr<DataStream>> _streams;
	QString _name;
};
