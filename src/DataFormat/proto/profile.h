#pragma once
#include <QString>
#include <QUuid>
#include <cstdint>

namespace Proto {

	struct Profile {
		QUuid userId;
		QString name;
	};


} // namespace Proto
