#include "Libs/Resources/directories/directories_context.h"

std::optional<QDir> DirectoriesContext::get(DirectoryPath path) const {
	if (!_directories.contains(path)) {
		return std::nullopt;
	}

	return _directories[path];
}

std::optional<QDir> DirectoriesContext::set(DirectoryPath path, const QDir& dir) {
	_directories[path] = dir;
	return dir;
}

QList<DirectoryPath> DirectoriesContext::available() const {
	return _directories.keys();
}

void DirectoriesContext::clear() {
	_directories.clear();
}

void DirectoriesContext::apply(const DirectoriesContext& values) {
	const auto keys = values.available();
	for (const auto it : keys) {
		auto value = values.get(it);
		if (!value) {
			continue;
		}

		_directories[it] = value.value();
	}
}

QStringList DirectoriesContext::paths() const {
	QStringList result;
	for (const auto key : _directories.keys()) {
		result.push_back(_directories[key].absolutePath());
	}

	return result;
}
