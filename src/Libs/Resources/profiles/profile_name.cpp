#include "Libs/Resources/profiles/profile_name.h"

#include <QRegularExpression>

namespace Profiles {

const QString& defaultName() {
	static const QString name = QStringLiteral("default");
	return name;
}

bool isValidName(const QString& name) {
	static const QRegularExpression pattern(QStringLiteral("^[a-z0-9_-]+$"));
	return !name.isEmpty() && pattern.match(name).hasMatch();
}

} // namespace Profiles
