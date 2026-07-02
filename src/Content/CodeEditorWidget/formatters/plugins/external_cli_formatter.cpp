#include "Content/CodeEditorWidget/formatters/plugins/external_cli_formatter.h"
#include "Content/CodeEditorWidget/language_info.h"

#include <QProcess>

class ExternalCliFormatterPlugin::Private {
public:
	Private(ExternalCliFormatterPlugin* parent) : q(parent) {}
	ExternalCliFormatterPlugin* q;

	QProcess* process = nullptr;
	QString formatterProgramm;
	QStringList arguments;
};

ExternalCliFormatterPlugin::ExternalCliFormatterPlugin(
	const QString& formatterProgramm,
	const QStringList& arguments)
	: d(std::make_unique<Private>(this)) {
	d->arguments = arguments;
	d->formatterProgramm = formatterProgramm;
}

ExternalCliFormatterPlugin::~ExternalCliFormatterPlugin() {
	stop();
}

QStringList ExternalCliFormatterPlugin::mimeTypes() const {
	return { 
		"application/json",
		"text/css",
		"text/html",
	};
}

void ExternalCliFormatterPlugin::formatAsync(const QString& text) {
	stop();

	d->process = new QProcess(this);

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

	d->process->start(d->formatterProgramm, d->arguments);

	// Передаем текст в stdin форматтера
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
