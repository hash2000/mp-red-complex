#include "Content/CodeEditorWidget/formatters/plugins/external_cli_formatter.h"
#include "Content/CodeEditorWidget/language_info.h"

#include <QProcess>
#include <QDir>
#include <QDebug>

class ExternalCliFormatterPlugin::Private {
public:
	Private(ExternalCliFormatterPlugin* parent) : q(parent) {}
	ExternalCliFormatterPlugin* q;

	QProcess* process = nullptr;
	QString formatterProgram;
	QString mimeType;
	QMap<QString, QString> arguments;
};

ExternalCliFormatterPlugin::ExternalCliFormatterPlugin(
	const QString& mimeType,
	const QString& formatterProgram,
	const QMap<QString, QString>& arguments)
	: d(std::make_unique<Private>(this)) {
	d->arguments = arguments;
	d->formatterProgram = formatterProgram;
	d->mimeType = mimeType;
}

ExternalCliFormatterPlugin::~ExternalCliFormatterPlugin() {
	stop();
}

QString ExternalCliFormatterPlugin::mimeType() const {
	return d->mimeType;
}

void ExternalCliFormatterPlugin::formatAsync(const QString& text) {
	stop();

	d->process = new QProcess(this);

	auto process = d->process;

	connect(process, &QProcess::readyReadStandardError, process, [process, mimeType = d->mimeType]() {
		qWarning() << mimeType << "Processing error:" << process->readAllStandardError();
	});

	connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
		QStringList message;
		message << "Process failed to start.";
		message << "Error:";

		switch (error) {
		case QProcess::FailedToStart: message << "Check if executable exists."; break;
		case QProcess::Crashed: message << "Process crashed."; break;
		case QProcess::Timedout: message << "Process timed out."; break;
		case QProcess::WriteError: message << "Write error."; break;
		case QProcess::ReadError: message << "Read error."; break;
		default: message << "Unknown error.";
		}

		const auto errString = process->errorString();
		message << errString;
		qWarning() << message;

		emit formattingFailed(message.join(" "));
	});

	connect(d->process, &QProcess::finished, this, [this, text](int exitCode, QProcess::ExitStatus status) {
		if (status == QProcess::NormalExit && exitCode == 0) {
			QString result = QString::fromUtf8(d->process->readAllStandardOutput());
			emit formattingFinished(result);
		}
		else {
			QString err = QString::fromUtf8(d->process->readAllStandardError());
			emit formattingFailed(err.isEmpty() ? "Formatting failed" : err);
		}
		d->process->deleteLater();
		d->process = nullptr;
	});

	QStringList argsList;
	for (auto it = d->arguments.begin(); it != d->arguments.end(); it++) {
		argsList << it.key();
		argsList << it.value();
	}

	// Устанавливаем переменные окружения (отключить цвета)
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert("NO_COLOR", "1");
	env.insert("FORCE_COLOR", "0");
	d->process->setProcessEnvironment(env);

	d->process->start(d->formatterProgram, argsList);
	d->process->write(text.toUtf8());
	d->process->closeWriteChannel();
}

void ExternalCliFormatterPlugin::stop() {
	if (d->process) {
		d->process->kill();
		d->process->deleteLater();
		d->process = nullptr;
	}
}
