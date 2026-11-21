#include "Content/FetchApiModule/models/fetch_api_types.h"

#include <QHash>
#include <QStringList>

namespace {

/// Позиция разделителя пары «имя → значение»: первый из `:` или `=`.
/// -1 — разделителя нет.
qsizetype separatorIndex(const QString& line) {
	const auto colon = line.indexOf(QLatin1Char(':'));
	const auto equals = line.indexOf(QLatin1Char('='));
	if (colon < 0) {
		return equals;
	}
	if (equals < 0) {
		return colon;
	}
	return qMin(colon, equals);
}

/// Разделитель списка пар: построчно, затем `;`, затем `&` (query-строка URL).
QChar valuesSeparator(const QString& text) {
	if (text.contains(QLatin1Char('\n'))) {
		return QLatin1Char('\n');
	}
	if (text.contains(QLatin1Char(';'))) {
		return QLatin1Char(';');
	}
	return QLatin1Char('&');
}

/// Имя заголовка в привычном виде: QNetworkReply отдаёт имена в нижнем
/// регистре, а пользователю и контексту нужен `Content-Type`, а не
/// `content-type`. Регистр имени в HTTP не значим, поэтому это оформление.
QString canonicalHeaderName(const QString& name) {
	static const QHash<QString, QString> known = {
		{ QStringLiteral("etag"), QStringLiteral("ETag") },
		{ QStringLiteral("www-authenticate"), QStringLiteral("WWW-Authenticate") },
		{ QStringLiteral("content-md5"), QStringLiteral("Content-MD5") },
		{ QStringLiteral("x-xss-protection"), QStringLiteral("X-XSS-Protection") },
		{ QStringLiteral("x-content-type-options"), QStringLiteral("X-Content-Type-Options") },
		{ QStringLiteral("x-frame-options"), QStringLiteral("X-Frame-Options") },
	};

	const auto lower = name.toLower();
	const auto it = known.constFind(lower);
	if (it != known.constEnd()) {
		return it.value();
	}

	QStringList parts = lower.split(QLatin1Char('-'));
	for (auto& part : parts) {
		if (!part.isEmpty()) {
			part[0] = part[0].toUpper();
		}
	}

	return parts.join(QLatin1Char('-'));
}

} // namespace

namespace FetchApiFormat {

FetchApiKeyValues parseKeyValues(const QString& text) {
	FetchApiKeyValues result;

	QString prepared = text.trimmed();
	if (prepared.startsWith(QLatin1Char('?'))) {
		prepared.remove(0, 1);
	}
	if (prepared.isEmpty()) {
		return result;
	}

	const auto separator = valuesSeparator(prepared);
	const auto lines = prepared.split(separator);
	for (const auto& rawLine : lines) {
		const auto line = rawLine.trimmed();
		if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
			continue;
		}

		const auto separatorPos = separatorIndex(line);
		if (separatorPos < 0) {
			continue;
		}

		const auto name = line.left(separatorPos).trimmed();
		if (name.isEmpty()) {
			continue;
		}

		result[name] = line.mid(separatorPos + 1).trimmed();
	}

	return result;
}

QString formatKeyValues(const FetchApiKeyValues& values) {
	QStringList lines;
	lines.reserve(static_cast<qsizetype>(values.size()));
	for (const auto& [name, value] : values) {
		lines << QStringLiteral("%1: %2").arg(name, value);
	}

	return lines.join(QLatin1Char('\n'));
}

FetchApiKeyValues headersMap(const QHttpHeaders& headers) {
	FetchApiKeyValues result;
	for (qsizetype i = 0; i < headers.size(); i++) {
		const auto name = headers.nameAt(i);
		const auto value = QString::fromUtf8(headers.valueAt(i));
		// Повторяющиеся заголовки (Set-Cookie) склеиваем: карта хранит
		// по одному значению, а полный список доступен через headerValues().
		const auto key = canonicalHeaderName(QString::fromLatin1(name));
		const auto it = result.find(key);
		if (it == result.end()) {
			result.emplace(key, value);
		}
		else {
			it->second = QStringLiteral("%1\n%2").arg(it->second, value);
		}
	}

	return result;
}

QString headerName(const QString& name) {
	return canonicalHeaderName(name);
}

QString formatHeaders(const QHttpHeaders& headers) {
	QStringList lines;
	lines.reserve(static_cast<qsizetype>(headers.size()));
	for (qsizetype i = 0; i < headers.size(); i++) {
		lines << QStringLiteral("%1: %2")
			.arg(canonicalHeaderName(QString::fromLatin1(headers.nameAt(i))),
				QString::fromUtf8(headers.valueAt(i)));
	}

	return lines.join(QLatin1Char('\n'));
}

QString headerValue(const QHttpHeaders& headers, const QString& name) {
	for (qsizetype i = 0; i < headers.size(); i++) {
		if (QString::fromLatin1(headers.nameAt(i)).compare(name, Qt::CaseInsensitive) == 0) {
			return QString::fromUtf8(headers.valueAt(i));
		}
	}

	return {};
}

QStringList headerValues(const QHttpHeaders& headers, const QString& name) {
	QStringList result;
	for (qsizetype i = 0; i < headers.size(); i++) {
		if (QString::fromLatin1(headers.nameAt(i)).compare(name, Qt::CaseInsensitive) == 0) {
			result << QString::fromUtf8(headers.valueAt(i));
		}
	}

	return result;
}

} // namespace FetchApiFormat
