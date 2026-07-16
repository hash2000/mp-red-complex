#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include <QFutureWatcher>
#include <QtConcurrent>

class FormatterPluginAsync::Private {
public:
	Private(FormatterPluginAsync* parent) : q(parent) {}
	FormatterPluginAsync* q;

	QFutureWatcher<QString>* watcher = nullptr;
};

FormatterPluginAsync::FormatterPluginAsync()
	: d(std::make_unique<Private>(this)) {
}

FormatterPluginAsync::~FormatterPluginAsync() {
	stop();
}

void FormatterPluginAsync::formatAsync(const QString& text) {
	stop();

	d->watcher = new QFutureWatcher<QString>(this);

	// Связываем сигнал завершения с нашим сигналом
	connect(d->watcher, &QFutureWatcher<QString>::finished, this, [this]() {
		if (!d->watcher->isCanceled()) {
			emit formattingFinished(d->watcher->result());
		}
		d->watcher->deleteLater();
		d->watcher = nullptr;
	});

	QFuture<QString> future = QtConcurrent::run([this](const QString& inputText) {
		QString formatted = internalFormat(inputText);
		return formatted;
	}, text);

	d->watcher->setFuture(future);
}

void FormatterPluginAsync::stop() {
	if (d->watcher) {
		d->watcher->cancel();
		d->watcher->deleteLater();
		d->watcher = nullptr;
	}
}
