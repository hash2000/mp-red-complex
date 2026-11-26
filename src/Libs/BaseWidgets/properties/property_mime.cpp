#include "Libs/BaseWidgets/properties/property_mime.h"

#include <QColor>
#include <QJsonDocument>

#include <limits>

namespace PropertyMime {

PropertyType editorType(const QString& mimeType) {
	const auto mime = mimeType.trimmed().toLower();
	if (mime == QLatin1String(PropertyMimeType::kFilePath)
		|| mime == QLatin1String(PropertyMimeType::kUriList)) {
		return PropertyType::PathFile;
	}
	if (mime == QLatin1String(PropertyMimeType::kDirectoryPath)) {
		return PropertyType::PathDir;
	}
	if (mime == QLatin1String(PropertyMimeType::kBoolean)) {
		return PropertyType::Boolean;
	}
	if (mime == QLatin1String(PropertyMimeType::kInteger)
		|| mime == QLatin1String(PropertyMimeType::kNumber)) {
		return PropertyType::Number;
	}
	if (mime == QLatin1String(PropertyMimeType::kColor)) {
		return PropertyType::Color;
	}
	if (mime == QLatin1String(PropertyMimeType::kMultiline)
		|| mime == QLatin1String(PropertyMimeType::kJson)
		|| mime == QLatin1String(PropertyMimeType::kXml)) {
		return PropertyType::Multiline;
	}

	return PropertyType::Text;
}

bool isInteger(const QString& mimeType) {
	return mimeType.trimmed().compare(QLatin1String(PropertyMimeType::kInteger),
		Qt::CaseInsensitive) == 0;
}

QString primaryMimeType(const QStringList& mimeTypes) {
	for (const auto& mimeType : mimeTypes) {
		const auto trimmed = mimeType.trimmed();
		if (!trimmed.isEmpty()) {
			return trimmed;
		}
	}

	return QString::fromLatin1(PropertyMimeType::kText);
}

namespace {

bool isServiceMimeType(const QString& mimeType) {
	return mimeType.compare(QLatin1String(PropertyMimeType::kShared),
			Qt::CaseInsensitive) == 0
		|| mimeType.compare(QLatin1String(PropertyMimeType::kContextVariable),
			Qt::CaseInsensitive) == 0
		|| mimeType.compare(QLatin1String(PropertyMimeType::kSecret),
			Qt::CaseInsensitive) == 0
		|| mimeType.compare(QLatin1String(PropertyMimeType::kHidden),
			Qt::CaseInsensitive) == 0;
}

bool containsMimeType(const QStringList& mimeTypes, const QString& mimeType) {
	for (const auto& raw : mimeTypes) {
		if (raw.trimmed().compare(mimeType, Qt::CaseInsensitive) == 0) {
			return true;
		}
	}

	return false;
}

} // namespace

QStringList valueMimeTypes(const QStringList& mimeTypes,
	const QStringList& ignoredMimeTypes) {
	QStringList result;
	for (const auto& raw : mimeTypes) {
		const auto mime = raw.trimmed();
		if (mime.isEmpty() || isServiceMimeType(mime)
			|| containsMimeType(ignoredMimeTypes, mime)) {
			continue;
		}

		if (!result.contains(mime, Qt::CaseInsensitive)) {
			result.append(mime);
		}
	}

	return result;
}

QString primaryValueMimeType(const QStringList& mimeTypes,
	const QStringList& ignoredMimeTypes) {
	return primaryMimeType(valueMimeTypes(mimeTypes, ignoredMimeTypes));
}

bool isSecret(const QStringList& mimeTypes) {
	return containsMimeType(mimeTypes, QLatin1String(PropertyMimeType::kSecret));
}

bool isHidden(const QStringList& mimeTypes) {
	return containsMimeType(mimeTypes, QLatin1String(PropertyMimeType::kHidden));
}

QStringList commonMimeTypes() {
	return {
		QString::fromLatin1(PropertyMimeType::kText),
		QString::fromLatin1(PropertyMimeType::kMultiline),
		QString::fromLatin1(PropertyMimeType::kFilePath),
		QString::fromLatin1(PropertyMimeType::kDirectoryPath),
		QString::fromLatin1(PropertyMimeType::kBoolean),
		QString::fromLatin1(PropertyMimeType::kInteger),
		QString::fromLatin1(PropertyMimeType::kNumber),
		QString::fromLatin1(PropertyMimeType::kColor),
		QString::fromLatin1(PropertyMimeType::kJson),
		QString::fromLatin1(PropertyMimeType::kXml),
		QString::fromLatin1(PropertyMimeType::kUriList),
		QStringLiteral("image/png"),
		QStringLiteral("image/jpeg"),
		QStringLiteral("application/octet-stream")
	};
}

QVariant defaultValue(const QString& mimeType) {
	switch (editorType(mimeType)) {
	case PropertyType::Boolean:
		return false;
	case PropertyType::Number:
		return isInteger(mimeType) ? QVariant(0) : QVariant(0.0);
	case PropertyType::Color:
		return QColor(Qt::black);
	default:
		return QString();
	}
}

QVariant normalizeValue(const QVariant& value, const QString& mimeType) {
	if (!value.isValid()) {
		return defaultValue(mimeType);
	}

	switch (editorType(mimeType)) {
	case PropertyType::Boolean:
		return value.toBool();
	case PropertyType::Number:
		return isInteger(mimeType)
			? QVariant(value.toLongLong())
			: QVariant(value.toDouble());
	case PropertyType::Color: {
		if (value.metaType().id() == QMetaType::QColor) {
			return value;
		}
		const QColor color(value.toString());
		return color.isValid() ? QVariant(color) : defaultValue(mimeType);
	}
	default:
		// Редактор текста: структурированные значения показываем как JSON,
		// бинарные — как текст UTF-8.
		if (value.metaType().id() == QMetaType::QVariantMap
			|| value.metaType().id() == QMetaType::QVariantList) {
			const QJsonDocument document = QJsonDocument::fromVariant(value);
			return QString::fromUtf8(document.toJson(QJsonDocument::Compact));
		}
		if (value.metaType().id() == QMetaType::QByteArray) {
			return QString::fromUtf8(value.toByteArray());
		}
		return value.toString();
	}
}

void applyNumberLimits(PropertyData& property, const QString& mimeType) {
	if (isInteger(mimeType)) {
		property.minVal = std::numeric_limits<int>::min();
		property.maxVal = std::numeric_limits<int>::max();
		property.step = 1;
	}
	else {
		property.minVal = -1e12;
		property.maxVal = 1e12;
		property.step = 0.001;
	}
}

} // namespace PropertyMime
