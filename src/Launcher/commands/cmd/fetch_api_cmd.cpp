#include "Launcher/commands/cmd/fetch_api_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/command_console/console_table.h"
#include "Launcher/commands/command_console/console_image.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Libs/Resources/resources.h"

#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/models/fetch_opt.h"

#include <QNetworkRequest>


class FetchApiCommand::Private {
public:
	Private(FetchApiCommand* parent) : q(parent) {}
	FetchApiCommand* q;

	bool sendRequest(CommandContext* context,
		const QString& action,
		const QString& location,
		const QString& headers,
		const QString& body
		);
};


FetchApiCommand::FetchApiCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, CommandAbstraction(parent) {
}

FetchApiCommand::~FetchApiCommand() = default;

QString FetchApiCommand::help() const {
	return R"(fetch-api
	action:[post,get,put,delete,...]
	location:{location}
	)";
}

bool FetchApiCommand::Private::sendRequest(CommandContext* context,
	const QString& action,
	const QString& location,
	const QString& headers,
	const QString& body
) {
	auto controller = context->controllers()->windowsController();
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
		opt.body = body.toUtf8();
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
		[context] (int statusCode, const QByteArray& data) {
			context->printSuccess(QString("fetch-api %1 %2 Kb")
				.arg(statusCode)
				.arg(data.length() / 1024));
		},
		[context] (int errorCode, const QString& errorString) {
			context->printError(QString("fetch-api %1 %2")
				.arg(errorCode)
				.arg(errorString));
		},
		[this] (qint64 received, qint64 total) {
			
		});

	return true;
}

bool FetchApiCommand::execute(CommandContext* context, const QStringList& args) {
	const auto action = parseArgsValue(args, "action");
	if (action.isEmpty()) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	const auto result = d->sendRequest(context, action,
		parseArgsValue(args, "location"),
		parseArgsValue(args, "headers"),
		parseArgsValue(args, "body"));

	return result;
}


