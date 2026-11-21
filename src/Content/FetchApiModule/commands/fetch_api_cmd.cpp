#include "Content/FetchApiModule/commands/fetch_api_cmd.h"
#include "Content/FetchApiModule/models/fetch_opt.h"
#include "Content/FetchApiModule/services/fetch_service.h"

#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/services/services_registry.h"

#include <QJsonDocument>
#include <QMap>
#include <QPointer>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QUuid>

namespace {

const char* kShowSslAction = "show-ssl";
const char* kSendAction = "send";
const char* kShowEditorVariable = "show-editor";

} // namespace

class FetchApiCommand::Private {
public:
	Private(FetchApiCommand* parent) : q(parent) { }
	FetchApiCommand* q;

	bool sendRequest(const std::shared_ptr<CommandContext>& context,
		const FetchApiOpt& options, bool showEditor);
	bool showSsl(const std::shared_ptr<CommandContext>& context);

	/// Контекст, в который пишется результат: документ, вызвавший команду,
	/// либо глобальный контекст, чтобы значения были видны всем командам
	/// и документам.
	static std::shared_ptr<CommandContext> resultContext(
		const std::shared_ptr<CommandContext>& context);

	/// Тело запроса из переменной контекста: JSON-блок, байты или текст.
	static QByteArray bodyFromVariable(const QVariant& value);

	/// Открыть тело ответа в code-editor (отдельный документ).
	static bool openResponseInEditor(const std::shared_ptr<CommandContext>& context,
		const FetchApiResult& result);

	/// Таблица «Status | Content-Type | Size | Time» для консоли.
	static ConsoleTable resultTable(const FetchApiResult& result);
};

FetchApiCommand::FetchApiCommand(QObject* parent)
	: ICommand(parent)
	, d(std::make_unique<Private>(this)) {
}

FetchApiCommand::~FetchApiCommand() = default;

QString FetchApiCommand::help() const {
	return QStringLiteral(
		"fetch-api action=send, location=<url>\n"
		"    [, method=get|post|put|delete|patch|head]\n"
		"    [, params=\"a=1; b=2\"]        query-параметры\n"
		"    [, headers=\"Name: value; Name2: value2\"]\n"
		"    [, body=<<json>>{\"x\":1}<</json>>]\n"
		"    [, ssl-policy=strict|trust-custom-ca|ignore-errors]\n"
		"    [, ssl-ca-file=<path>][, handler=rest][, show-editor=true]\n"
		"fetch-api action=show-ssl\n"
		"fetch-api action=get, location=<url>   — метод прямо в action\n"
		"Результат пишется в контекст: data, content-type, headers,\n"
		"status-code, status-text, error, error-code, elapsed-ms, size, url.\n"
		"Событие контекста по завершении: fetch-api.complete.");
}

std::shared_ptr<CommandContext> FetchApiCommand::Private::resultContext(
	const std::shared_ptr<CommandContext>& context) {
	if (!context) {
		return nullptr;
	}

	if (auto document = context->owningDocumentContext()) {
		return document;
	}

	return context->globalContext();
}

QByteArray FetchApiCommand::Private::bodyFromVariable(const QVariant& value) {
	if (!value.isValid()) {
		return {};
	}

	if (value.metaType() == QMetaType::fromType<QJsonDocument>()) {
		return value.toJsonDocument().toJson(QJsonDocument::Compact);
	}

	if (value.metaType() == QMetaType::fromType<QByteArray>()) {
		return value.toByteArray();
	}

	return value.toString().toUtf8();
}

ConsoleTable FetchApiCommand::Private::resultTable(const FetchApiResult& result) {
	ConsoleTable table({ QStringLiteral("Status"), QStringLiteral("Content-Type"),
		QStringLiteral("Size"), QStringLiteral("Time") });

	const auto status = result.statusCode > 0
		? QStringLiteral("%1 %2").arg(result.statusCode).arg(result.statusText).trimmed()
		: QStringLiteral("—");
	const auto contentType = result.contentType().isEmpty()
		? QStringLiteral("—") : result.contentType();

	table.addRow({
		status,
		contentType,
		QStringLiteral("%1 B").arg(result.size),
		QStringLiteral("%1 ms").arg(result.elapsedMs)
		});

	return table;
}

bool FetchApiCommand::Private::openResponseInEditor(
	const std::shared_ptr<CommandContext>& context, const FetchApiResult& result) {
	auto* controller = context ? context->commandController() : nullptr;
	if (!controller) {
		return false;
	}

	// Документ создаётся штатной командой зоны: код-редактор получает `data`
	// и работает в режиме данных (файл не требуется).
	const QMap<QString, QString> arguments = {
		{ QStringLiteral("action"), QStringLiteral("create") },
		{ QStringLiteral("target"), QStringLiteral("code-editor") },
		{ QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces) },
		{ QStringLiteral("title"), QStringLiteral("fetch-api %1").arg(result.statusCode) },
		{ QStringLiteral("data"), result.text() },
	};

	return controller->executeCommand(QStringLiteral("documents"), arguments);
}

bool FetchApiCommand::Private::sendRequest(const std::shared_ptr<CommandContext>& context,
	const FetchApiOpt& options, bool showEditor) {
	auto* controller = context->commandController();
	auto* services = controller->services();
	auto* service = services ? services->get<FetchApiService>() : nullptr;
	if (!service) {
		controller->printError(QStringLiteral(
			"fetch-api: сервис выполнения запросов недоступен"));
		return false;
	}

	const auto target = Private::resultContext(context);
	const auto description = options.describe();

	// Контроллер может быть уничтожен раньше, чем придёт ответ; контекст
	// результата держим shared_ptr — он переживёт документ.
	QPointer<CommandController> guard(controller);
	const bool started = service->execute(options,
		[guard, target, showEditor, description](const FetchApiResult& result) {
			if (target) {
				result.applyToContext(*target);
			}

			if (!guard) {
				return;
			}

			if (result.failed) {
				guard->printError(QStringLiteral("fetch-api: %1. %2")
					.arg(description, result.errorMessage()));
			}
			else {
				guard->printSuccess(QStringLiteral("fetch-api: %1").arg(description));
			}

			guard->print(Private::resultTable(result));

			if (!showEditor) {
				return;
			}

			if (result.data.isEmpty()) {
				guard->printWarning(QStringLiteral(
					"fetch-api: тело ответа пусто, редактор не открыт"));
				return;
			}

			if (!Private::openResponseInEditor(target, result)) {
				guard->printError(QStringLiteral(
					"fetch-api: не удалось открыть ответ в редакторе"));
			}
		});

	if (!started) {
		controller->printError(QStringLiteral("fetch-api: %1").arg(service->lastError()));
		return false;
	}

	controller->print(QStringLiteral("fetch-api: запрос отправлен — %1").arg(description));
	return true;
}

bool FetchApiCommand::Private::showSsl(const std::shared_ptr<CommandContext>& context) {
	auto* controller = context->commandController();

	ConsoleTable table({ QStringLiteral("Option"), QStringLiteral("Value") });
	table.addRow({ QStringLiteral("SSL support"),
		QSslSocket::supportsSsl() ? QStringLiteral("yes") : QStringLiteral("no") });
	table.addRow({ QStringLiteral("Backend"), QSslSocket::activeBackend() });
	table.addRow({ QStringLiteral("OpenSSL build"), QSslSocket::sslLibraryBuildVersionString() });
	table.addRow({ QStringLiteral("OpenSSL runtime"), QSslSocket::sslLibraryVersionString() });
	table.addRow({ QStringLiteral("System CA certificates"),
		QString::number(QSslConfiguration::defaultConfiguration().caCertificates().size()) });
	table.addRow({ QStringLiteral("Policies"),
		QStringLiteral("strict, trust-custom-ca, ignore-errors") });

	controller->print(table);
	return true;
}

bool FetchApiCommand::execute(std::shared_ptr<CommandContext> context) {
	if (!context || !context->commandController()) {
		return false;
	}

	auto* controller = context->commandController();
	const auto action = context->variable(QStringLiteral("action"))
		.toString().trimmed().toLower();
	if (action.isEmpty()) {
		controller->printError(QStringLiteral("Использование:\n%1").arg(help()));
		return false;
	}

	if (action == QLatin1String(kShowSslAction)) {
		return d->showSsl(context);
	}

	// action — либо действие send, либо сам HTTP-метод (сокращённая запись).
	auto methodText = context->variable(QString::fromLatin1(FetchApiVariables::kMethod))
		.toString().trimmed();
	if (action != QLatin1String(kSendAction)) {
		const auto parsed = From<FetchApiMethod>::from(action);
		if (!parsed) {
			controller->printError(QStringLiteral("fetch-api: неизвестное действие «%1»\n%2")
				.arg(action, help()));
			return false;
		}

		if (methodText.isEmpty()) {
			methodText = action;
		}
	}

	if (methodText.isEmpty()) {
		methodText = QStringLiteral("get");
	}

	FetchApiOpt options;

	auto location = context->variable(QString::fromLatin1(FetchApiVariables::kLocation))
		.toString().trimmed();
	if (location.isEmpty()) {
		location = context->variable(QString::fromLatin1(FetchApiVariables::kUrlAlias))
			.toString().trimmed();
	}
	options.setLocation(location);

	if (!options.setMethod(methodText)) {
		controller->printError(QStringLiteral("fetch-api: неизвестный HTTP-метод «%1»")
			.arg(methodText));
		return false;
	}

	const auto handlerText = context->variable(QString::fromLatin1(FetchApiVariables::kHandler))
		.toString();
	if (!options.setHandler(handlerText)) {
		controller->printError(QStringLiteral(
			"fetch-api: обработчик «%1» не поддерживается").arg(handlerText));
		return false;
	}

	auto params = context->variable(QString::fromLatin1(FetchApiVariables::kParams)).toString();
	if (params.isEmpty()) {
		params = context->variable(QString::fromLatin1(FetchApiVariables::kParamsAlias)).toString();
	}
	options.setParametersFormatted(params);
	options.setHeadersFormatted(context->variable(
		QString::fromLatin1(FetchApiVariables::kHeaders)).toString());

	// Content-Type можно задать отдельной переменной — это удобно для JSON-тела.
	const auto contentType = context->variable(
		QString::fromLatin1(FetchApiVariables::kContentType)).toString();
	if (!contentType.isEmpty() && !options.hasHeader(QStringLiteral("Content-Type"))) {
		options.addHeader(QStringLiteral("Content-Type"), contentType);
	}

	options.setBody(Private::bodyFromVariable(context->variable(
		QString::fromLatin1(FetchApiVariables::kBody))));

	const auto sslPolicyText = context->variable(
		QString::fromLatin1(FetchApiVariables::kSslPolicy)).toString();
	if (!options.setSslPolicy(sslPolicyText)) {
		controller->printError(QStringLiteral("fetch-api: неизвестная политика TLS «%1»")
			.arg(sslPolicyText));
		return false;
	}

	const auto caFile = context->variable(
		QString::fromLatin1(FetchApiVariables::kSslCaFile)).toString();
	if (!caFile.isEmpty()) {
		QString error;
		if (!options.loadCustomCaFromFile(caFile, &error)) {
			controller->printError(QStringLiteral("fetch-api: %1").arg(error));
			return false;
		}
	}

	const auto showEditorText = context->variable(QString::fromLatin1(kShowEditorVariable))
		.toString().trimmed().toLower();
	const bool showEditor = showEditorText == QStringLiteral("true")
		|| showEditorText == QStringLiteral("1")
		|| showEditorText == QStringLiteral("yes");

	if (!options.isValid()) {
		controller->printError(location.isEmpty()
			? QStringLiteral("fetch-api: не задан адрес запроса (location)")
			: QStringLiteral("fetch-api: некорректный адрес запроса: %1").arg(location));
		return false;
	}

	return d->sendRequest(context, options, showEditor);
}
