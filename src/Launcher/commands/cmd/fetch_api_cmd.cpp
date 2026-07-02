#include "Launcher/commands/cmd/fetch_api_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/command_console/console_table.h"
#include "Launcher/commands/command_console/console_image.h"
#include "Launcher/commands/command_console/console_json.h"
#include "Launcher/commands/instruction.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Libs/Resources/resources.h"

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

	bool sendRequest(CommandContext* context,
		const QString& action,
		const QString& location,
		const QString& headers,
		const QByteArray& body,
		bool showEditor
		);

	bool showSsl(CommandContext* context);
};


FetchApiCommand::FetchApiCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, CommandAbstraction(parent) {
}

FetchApiCommand::~FetchApiCommand() = default;

QString FetchApiCommand::help() const {
	return R"(fetch-api
	action:
	show-ssl
	[post,get,put,delete,...]
		location:{location}
	)";
}

bool FetchApiCommand::Private::sendRequest(CommandContext* context,
	const QString& action,
	const QString& location,
	const QString& headers,
	const QByteArray& body,
	bool showEditor
) {
	auto services = context->services();
	auto fetchService = services->fetchApiService();

	if (location.isEmpty()) {
		context->printError("fetch-api. Empty location");
		return false;
	}

	FetchApiOpt opt;

	opt.action = FetchApiActions::Undefined;
	if (action == "get") opt.action = FetchApiActions::Get;
	else if (action == "post") opt.action = FetchApiActions::Post;
	else if (action == "put") opt.action = FetchApiActions::Put;
	else if (action == "delete") opt.action = FetchApiActions::Delete;
	else {
		context->printError(QString("fetch-api. Unknown action. %1")
			.arg(action));
		return false;
	}

	if (opt.action != FetchApiActions::Get && !body.isEmpty()) {
		opt.body = body;
	}

	opt.request = QNetworkRequest(location);

	const auto headersParsed = headers.split(";", Qt::SkipEmptyParts);
	for (const auto hdr : headersParsed) {
		const auto kv = hdr.split(":", Qt::SkipEmptyParts);
		if (kv.isEmpty() || kv.count() != 2) {
			continue;
		}

		opt.request.setRawHeader(kv[0].toUtf8(), kv[1].toUtf8());
	}

	fetchService->fetchRequest(opt,
		[context, showEditor] (int statusCode, const QByteArray& data, const QHttpHeaders& headers) {
			QString lang = "txt";
			const auto contentType = QString::fromUtf8(headers.value("Content-Type"));
			if (contentType.contains("application/json")) {
				lang = "json";
				ConsoleJson json(data);
				if (!json.isValid()) {
					context->printError(json.getErrorMessage());
					return;
				}
				context->print(json);
			}
			else if (contentType.contains("text/html")) {
				lang = "html";
			}

			context->printSuccess(QString("fetch-api %1 %2 bytes")
				.arg(statusCode)
				.arg(data.length()));
			if (showEditor) {
				context->applicationController()->executeCommand("window-create", {
					{ "action", "create" },
					{ "target", "code-editor" },
					{ "format-document", "true" },
					{ "lang", lang },
					{ "text", QString::fromUtf8(data) },
					});
			}
		},
		[context] (int errorCode, const QString& errorString, const QHttpHeaders& headers) {
			context->printError(QString("fetch-api %1 %2")
				.arg(errorCode)
				.arg(errorString));
		},
		[this] (qint64 received, qint64 total) {
			
		});

	return true;
}

bool FetchApiCommand::Private::showSsl(CommandContext* context) {
	ConsoleTable table({ "Option", "Value" });
	table.addRow({ "SSL Support", QSslSocket::supportsSsl() });
	table.addRow({ "Backend", QSslSocket::activeBackend() });
	table.addRow({ "OpenSSL build", QSslSocket::sslLibraryBuildVersionString() });
	table.addRow({ "OpenSSL runtime", QSslSocket::sslLibraryVersionString() });

	context->print(table);
	return true;
}

bool FetchApiCommand::execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "show-ssl") return d->showSsl(context);

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


