#pragma once
#include "DataStream/data_stream.h"
#include "Base/container_view.h"
#include <QString>
#include <map>
#include <memory>
#include <optional>

class DataStreamContainer {
public:
	virtual ~DataStreamContainer() = default;

	virtual ContainerType type() const = 0;

	const QString name() const;
	void name(const QString &name);

	auto items() const {
    return make_deref_view(_streams);
	}

	auto find(const QString &name) const
		-> std::optional<std::shared_ptr<DataStream>>;

protected:
	void add(std::shared_ptr<DataStream> stream);
	void remove(const QString &name);
	void clear();

private:
	std::map<QString, std::shared_ptr<DataStream>> _streams;
	QString _name;
};
