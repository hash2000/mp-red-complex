#pragma once
#include "Base/config.h"
#include "Resources/resources.h"
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

	void configure(const std::shared_ptr<Config> &config) override;

	void setup(std::shared_ptr<Config> &config, std::shared_ptr<Resources> &resources);

	void setupMainFrame(std::unique_ptr<MainFrame> &&mainFrame);

private:
	bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

	void registerTypes();

private:
	std::unique_ptr<MainFrame> _main_frame;
};
