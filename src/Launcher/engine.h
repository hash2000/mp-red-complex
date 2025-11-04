#pragma once
#include "Launcher/config.h"
#include "Launcher/resources.h"
#include "Launcher/main_frame.h"
#include <QApplication>
#include <qabstractnativeeventfilter.h>
#include <memory>

class Engine
	: public QApplication
	, private QAbstractNativeEventFilter
	, public Configurable
{
public:
	Engine(int &argc, char **argv);

	virtual ~Engine() = default;

	void configure(const Config& config) override;

	void setup(Config& config, Resources& resources);

	void setupMainFrame(std::unique_ptr<MainFrame> &&mainFrame);

private:
	bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
	std::unique_ptr<MainFrame> _main_frame;
};
