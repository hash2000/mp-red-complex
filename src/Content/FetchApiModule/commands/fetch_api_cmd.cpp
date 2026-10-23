#include "Content/FetchApiModule/commands/fetch_api_cmd.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_image.h"
#include "Content/ConsoleModule/command_console/console_json.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Resources/resources.h"
#include "Libs/Engine/services/services_registry.h"

#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/models/fetch_opt.h"

#include <QNetworkRequest>
#include <QSslSocket>
#include <QJsonDocument>
#include <QHttpHeaders>


class FetchApiCommand::Private {
public:
	Private(FetchApiCommand* parent) : q(parent) {}
	FetchApiCommand* q;

	bool sendRequest(std::shared_ptr<CommandContext> context,
		const QString& method,
		const QString& location,
		const QString& headers,
		const QByteArray& body,
		bool showEditor
		);

	bool showSsl(std::shared_ptr<CommandContext> context);
};


FetchApiCommand::FetchApiCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, ICommand(parent) {
}

FetchApiCommand::~FetchApiCommand() = default;

QString FetchApiCommand::help() const {
	return R"(fetch-api action:
	show-ssl
	[post,get,put,delete,...]
		location:{location}
	)";
}

bool FetchApiCommand::Private::sendRequest(std::shared_ptr<CommandContext> context,
	const QString& method,
	const QString& location,
	const QString& headers,
	const QByteArray& body,
	bool showEditor
) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto fetchService = services->get<FetchApiService>();

	if (location.isEmpty()) {
		controller->printError("fetch-api. Empty location");
		return false;
	}

	FetchApiOpt opt(location, std::map<QString, QString>(), headers);

	if (!opt.setMethod(method)) {
		controller->printError(QString("fetch-api. Unknown action. %1")
			.arg(method));
		return false;
	}
	opt.setBody(body);

	fetchService->fetchRequest(opt,
		[controller, showEditor] (int statusCode, const QByteArray& data, const QHttpHeaders& headers) {
			QString contentType = QString::fromUtf8(headers.value("Content-Type"));
			controller->printSuccess(QString("fetch-api %1 %2 bytes")
				.arg(statusCode)
				.arg(data.length()));
			if (showEditor) {
				controller->executeCommand("windows", {
					{ "action", "create" },
					{ "target", "code-editor" },
					{ "format-document", "true" },
					{ "content-type", contentType },
					{ "text", QString::fromUtf8(data) },
					});
			}
		},
		[controller] (int errorCode, const QString& errorString, const QHttpHeaders& headers) {
		controller->printError(QString("fetch-api %1 %2")
				.arg(errorCode)
				.arg(errorString));
		},
		[this] (qint64 received, qint64 total) {
			
		});

	return true;
}

bool FetchApiCommand::Private::showSsl(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	ConsoleTable table({ "Option", "Value" });
	table.addRow({ "SSL Support", QSslSocket::supportsSsl() });
	table.addRow({ "Backend", QSslSocket::activeBackend() });
	table.addRow({ "OpenSSL build", QSslSocket::sslLibraryBuildVersionString() });
	table.addRow({ "OpenSSL runtime", QSslSocket::sslLibraryVersionString() });

	controller->print(table);
	return true;
}

bool FetchApiCommand::execute(const std::shared_ptr<Instruction> instruction, std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		controller->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "show-ssl") {
		return d->showSsl(context);
	}

	const auto showEditor = instruction->text("show-editor", "false") != "false";
	auto body = instruction->parameter("body")
		.toJsonDocument()
		.toJson(QJsonDocument::Compact);

	const auto result = d->sendRequest(context, action,
		instruction->text("location"),
		instruction->text("headers"),
		body,
		showEditor);

	return result;
}


