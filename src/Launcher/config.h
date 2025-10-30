#pragma once
#include <QStyleHints>

class Config {
public:
	Config();

	Config(const Config& clone);

	virtual ~Config() = default;

	static const Config& getDefult();

public:
	Qt::ColorScheme color_scheme;
};
