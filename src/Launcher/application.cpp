#include "Launcher/application.h"
#include "Launcher/application/application_exception.h"
#include "Launcher/engine.h"
#include "main_frame/graphwidget/main_frame_test.h"
#include "main_frame/resources_viewer/resources_viewer_frame.h"

std::unique_ptr<Application> Application::create() {
  return std::make_unique<Application>();
}

int Application::run(int &argc, char **argv) {
	try {
		return tryRun(argc, argv);
	}
	catch(std::exception &ex) {

	}

	return 0;
}

int Application::tryRun(int &argc, char **argv) {
	_config = std::make_unique<Config>(Config::getDefult());

	Engine engine(argc, argv);
	engine.configure(*_config);
	engine.configSetup(*_config);

	installMessageHandler();

	std::unique_ptr<MainFrame> mainFrame;
	switch (_config->app_session) {
		case AppSession::Test:
			mainFrame = std::make_unique<MainFrameTest>();
			break;
		case AppSession::ResourcesView:
			mainFrame = std::make_unique<ResourcesViewerFrame>();
			break;
		default:
			throw ApplicationExceptionUndefinedMainFrame();
	}

	mainFrame->configure(*_config);
	engine.setupMainFrame(std::move(mainFrame));

	return engine.exec();
}

void Application::installMessageHandler() {
	static QtMessageHandler originalMessageHandler = nullptr;
	originalMessageHandler = qInstallMessageHandler([](QtMsgType type,
			const QMessageLogContext &context,
			const QString &message) {
				if (originalMessageHandler) {
					originalMessageHandler(type, context, message);
				}

				const auto msg = qFormatLogMessage(type, context, message);

			});
}
