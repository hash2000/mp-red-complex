#pragma once
#include "Resources/resources/data_stream.h"
#include "Base/container_view.h"
#include <QString>
#include <map>
#include <memory>
#include <optional>
#include <functional>

class DataStreamContainer {
public:
	virtual ~DataStreamContainer() = default;

	const QString name() const;

	void name(const QString &name);

	auto items() const {
    return make_unique_ptr_view(_streams);
	}

	auto find(const QString &name) const
		-> std::optional<std::reference_wrapper<DataStream>>;

protected:
	void add(std::unique_ptr<DataStream> &&stream);

	void remove(const QString &name);

	void clear();

private:
	std::map<QString, std::unique_ptr<DataStream>> _streams;
	QString _name;
};
