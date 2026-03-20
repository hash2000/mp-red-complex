#pragma once
#include "DataStream/data_stream.h"
#include "Base/container_view.h"
#include <memory>
#include <optional>
#include <map>

class DataStreamContainer {
public:
	virtual ~DataStreamContainer() = default;

	virtual ContainerType type() const = 0;

	const QString name() const;
	void name(const QString &name);

	virtual std::map<QString, std::shared_ptr<DataStream>> items() const = 0;

	virtual std::optional<std::shared_ptr<DataStream>> find(const QString &name) const = 0;

protected:
	virtual void add(std::shared_ptr<DataStream> stream) = 0;
	virtual void remove(const QString &name) = 0;
	virtual void clear() = 0;

private:
	QString _name;
};
