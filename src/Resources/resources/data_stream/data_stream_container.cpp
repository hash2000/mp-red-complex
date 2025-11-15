#include "Resources/resources/data_stream/data_stream_container.h"

void DataStreamContainer::add(std::unique_ptr<DataStream> &&stream) {
	const auto name = stream->name();
	remove(name);
	_streams.emplace(name, std::move(stream));
}

void DataStreamContainer::remove(const QString &name) {
	auto it = _streams.find(name);
	if (it == _streams.end()) {
		return;
	}

	_streams.erase(it);
}

void DataStreamContainer::clear() {
	_streams.clear();
}

const QString DataStreamContainer::name() const {
  return _name;
}

void DataStreamContainer::name(const QString &name) {
	_name = name;
}

auto DataStreamContainer::find(const QString &name) const
	-> std::optional<std::reference_wrapper<DataStream>> {
	const auto &it = _streams.find(name);
	if (it == _streams.end()) {
		return std::nullopt;
	}

	return std::ref(*it->second);
}
