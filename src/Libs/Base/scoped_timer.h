#pragma once
#include <QElapsedTimer>

class ScopedTimer {
public:
	explicit ScopedTimer(const QString& lbl = "Operation")
	: _label(lbl) {
		qDebug().noquote() << _label << "Start.";
		_timer.start();
	}

	~ScopedTimer() {
		qDebug().noquote() << _label << "took" << _timer.elapsed() << "ms." << _result;
	}

	void ok() {
		_result = "Ok.";
	}

	void error(const QString& err) {
		_result = QString("Error: %1.")
			.arg(err);
	}

private:
	QElapsedTimer _timer;
	QString _label;
	QString _result{ "Ok." };
};
