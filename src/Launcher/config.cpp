#include "config.h"

const Config _default_scheme;

Config::Config()
	: color_scheme(Qt::ColorScheme::Dark)
{
}

Config::Config(const Config &clone) {
	color_scheme = clone.color_scheme;
}

const Config &Config::getDefult() {
	return _default_scheme;
}
