#pragma once
#include <QApplication>
#include <qabstractnativeeventfilter.h>
#include "config.h"

class Engine
	: public QApplication
	, private QAbstractNativeEventFilter
{
public:
	Engine(int &argc, char **argv);

	virtual ~Engine() = default;

	void configure(const Config& config);

private:
	bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
};
