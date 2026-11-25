#include "Libs/BaseWidgets/editor/formatters/formatter_plugin.h"
#include <QFutureWatcher>
#include <QtConcurrent>

class FormatterPluginAsync::Private {
public:
	Private(FormatterPluginAsync* parent) : q(parent) {}
	FormatterPluginAsync* q;

	QFutureWatcher<QString>* watcher = nullptr;
	/// Номер запуска: результат устаревшего запуска не должен попадать
	/// в редактор (QFutureWatcher::cancel() не останавливает вычисление).
	int generation = 0;
};

FormatterPluginAsync::FormatterPluginAsync()
	: d(std::make_unique<Private>(this)) {
}

FormatterPluginAsync::~FormatterPluginAsync() {
	stop();
}

void FormatterPluginAsync::formatAsync(const QString& text) {
	stop();

	if (!d->watcher) {
		d->watcher = new QFutureWatcher<QString>(this);
	}

	auto* watcher = d->watcher;
	const int generation = d->generation;

	// Связываем сигнал завершения с нашим сигналом
	connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, generation]() {
		if (generation != d->generation || watcher->isCanceled()) {
			// Устаревший запуск: watcher уже удаляется из stop().
			return;
		}

		const QString result = watcher->result();
		watcher->deleteLater();
		if (d->watcher == watcher) {
			d->watcher = nullptr;
		}
		emit formattingFinished(result);
	});

	QFuture<QString> future = QtConcurrent::run([this](const QString& inputText) {
		QString formatted = internalFormat(inputText);
		return formatted;
	}, text);

	watcher->setFuture(future);
}

void FormatterPluginAsync::stop() {
	++d->generation;
	if (d->watcher) {
		d->watcher->cancel();
		d->watcher->deleteLater();
		d->watcher = nullptr;
	}
}
