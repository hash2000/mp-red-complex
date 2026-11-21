#include "Content/FetchApiModule/models/fetch_api_result.h"
#include "Libs/Engine/scripting/command_context.h"

namespace {

const QString kContentTypeHeader = QStringLiteral("Content-Type");
const QString kSetCookieHeader = QStringLiteral("Set-Cookie");

} // namespace

bool FetchApiResult::isSuccess() const {
	return !failed && statusCode >= 200 && statusCode < 300;
}

bool FetchApiResult::isHttpError() const {
	return statusCode >= 400;
}

QString FetchApiResult::text() const {
	return QString::fromUtf8(data);
}

QString FetchApiResult::contentType() const {
	return FetchApiFormat::headerValue(headers, kContentTypeHeader);
}

QString FetchApiResult::headersFormatted() const {
	return FetchApiFormat::formatHeaders(headers);
}

QList<QPair<QString, QString>> FetchApiResult::cookies() const {
	QList<QPair<QString, QString>> result;
	for (const auto& raw : FetchApiFormat::headerValues(headers, kSetCookieHeader)) {
		const auto parts = raw.split(QLatin1Char(';'));
		if (parts.isEmpty()) {
			continue;
		}

		const auto nameValue = parts.first().trimmed();
		const auto name = nameValue.section(QLatin1Char('='), 0, 0).trimmed();
		if (name.isEmpty()) {
			continue;
		}

		QStringList attributes;
		for (qsizetype i = 1; i < parts.size(); i++) {
			const auto attribute = parts.at(i).trimmed();
			if (!attribute.isEmpty()) {
				attributes << attribute;
			}
		}

		result.append({ name, attributes.join(QStringLiteral("; ")) });
	}

	return result;
}

QString FetchApiResult::errorMessage() const {
	if (!errorString.isEmpty()) {
		return errorString;
	}

	if (isHttpError()) {
		return statusText.isEmpty()
			? QStringLiteral("HTTP %1").arg(statusCode)
			: QStringLiteral("HTTP %1 %2").arg(statusCode).arg(statusText);
	}

	return {};
}

void FetchApiResult::applyToContext(CommandContext& context) const {
	// Контент и его тип: по content-type консоль и панель свойств понимают,
	// как показывать значение.
	const auto content = text();
	const auto type = contentType();
	context.setVariable(FetchApiVariables::kData, content);
	context.setVariableMimeType(FetchApiVariables::kData, type);

	context.setVariable(FetchApiVariables::kContentType, type);
	context.setVariable(FetchApiVariables::kHeaders, headersFormatted());
	context.setVariable(FetchApiVariables::kStatusCode, statusCode);
	context.setVariable(FetchApiVariables::kStatusText, statusText);
	context.setVariable(FetchApiVariables::kError, failed ? errorMessage() : QString());
	context.setVariable(FetchApiVariables::kErrorCode, errorCode);
	context.setVariable(FetchApiVariables::kElapsedMs, elapsedMs);
	context.setVariable(FetchApiVariables::kSize, size);
	context.setVariable(FetchApiVariables::kUrl, url.isValid() ? url.toString() : location);
	context.setVariable(FetchApiVariables::kMethod,
		Format<FetchApiMethod>::format(method));

	context.fireEvent(completeEventName());
}

const QString& FetchApiResult::completeEventName() {
	static const QString event = QString::fromLatin1(FetchApiVariables::kCompleteEvent);
	return event;
}
