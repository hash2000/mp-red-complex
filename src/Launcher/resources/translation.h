#pragma once
#include <QDir>

class Translation {
public:
	Translation(const QDir &path);

	virtual ~Translation() = default;

	void load();

	void unload();

private:
	bool _loaded = false;
	const QDir _path;
};
