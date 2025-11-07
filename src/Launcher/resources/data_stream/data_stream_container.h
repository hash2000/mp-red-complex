#pragma once
#include "Launcher/resources/data_stream.h"
#include "Base/container_view.h"
#include <QString>
#include <map>
#include <memory>

class DataStreamContainer {
public:
	virtual ~DataStreamContainer() = default;

	const QString name() const;

	void name(const QString &name);

	auto items() const {
    return make_unique_ptr_view(_streams);
	}

protected:
	void add(std::unique_ptr<DataStream> &&stream);

	void remove(const QString &name);

	void clear();

private:
	std::map<QString, std::unique_ptr<DataStream>> _streams;
	QString _name;
};
