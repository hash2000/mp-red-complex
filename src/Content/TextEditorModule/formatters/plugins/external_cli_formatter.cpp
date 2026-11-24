#include "Content/TextEditorModule/formatters/plugins/external_cli_formatter.h"
#include "Content/TextEditorModule/language_info.h"

#include <QDebug>
#include <QDir>
#include <QProcess>

class ExternalCliFormatterPlugin::Private {
public:
	Private(ExternalCliFormatterPlugin* parent) : q(parent) {}
	ExternalCliFormatterPlugin* q;

	QProcess* process = nullptr;
	QString formatterProgram;
	QString mimeType;
	QStringList leadingArguments;
	QMap<QString, QString> arguments;
};

ExternalCliFormatterPlugin::ExternalCliFormatterPlugin(
	const QString& mimeType,
	const QString& formatterProgram,
	const QMap<QString, QString>& arguments,
	const QStringList& leadingArguments)
	: d(std::make_unique<Private>(this)) {
	d->arguments = arguments;
	d->leadingArguments = leadingArguments;
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
	QProcess* process = d->process;

	connect(process, &QProcess::readyReadStandardError, process, [process, mimeType = d->mimeType]() {
		qWarning() << mimeType << "Processing error:" << process->readAllStandardError();
	});

	connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
		if (d->process != process) {
			return;
		}

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

		message << process->errorString();
		qWarning() << message;

		emit formattingFailed(message.join(" "));
	});

	connect(process, &QProcess::finished, this,
		[this, process](int exitCode, QProcess::ExitStatus status) {
			// Процесс мог быть остановлен (stop()) и уже не быть текущим.
			if (d->process != process) {
				return;
			}

			d->process = nullptr;

			if (status == QProcess::NormalExit && exitCode == 0) {
				emit formattingFinished(QString::fromUtf8(process->readAllStandardOutput()));
			}
			else {
				const QString error = QString::fromUtf8(process->readAllStandardError());
				emit formattingFailed(error.isEmpty() ? QStringLiteral("Formatting failed") : error);
			}

			process->deleteLater();
		});

	QStringList argsList = d->leadingArguments;
	for (auto it = d->arguments.begin(); it != d->arguments.end(); it++) {
		argsList << it.key();
		argsList << it.value();
	}

	// Устанавливаем переменные окружения (отключить цвета)
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert("NO_COLOR", "1");
	env.insert("FORCE_COLOR", "0");
	process->setProcessEnvironment(env);

	process->start(d->formatterProgram, argsList);

	// QProcess буферизует запись до фактического запуска процесса.
	process->write(text.toUtf8());
	process->closeWriteChannel();
}

void ExternalCliFormatterPlugin::stop() {
	if (!d->process) {
		return;
	}

	// Отключаем сигналы до kill(): finished придёт асинхронно, когда
	// указатель на процесс уже сброшен.
	QProcess* process = d->process;
	d->process = nullptr;
	process->disconnect(this);
	process->kill();
	process->deleteLater();
}
