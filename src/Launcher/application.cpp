#include "Launcher/application.h"
#include "Launcher/application/application_session.h"
#include "Launcher/application/application_exception.h"
#include "Launcher/engine.h"
#include "main_frame/graphwidget/main_frame_test.h"
#include "main_frame/resources_viewer/resources_viewer_frame.h"
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QSurfaceFormat>

std::unique_ptr<Application> Application::create() {
  return std::make_unique<Application>();
}

int Application::run(int &argc, char **argv) {
	try {
		return tryRun(argc, argv);
	}
	catch(std::exception &ex) {
		qFatal() << "Application fatal exception: " << ex.what();
	}

	return 0;
}

int Application::tryRun(int &argc, char **argv) {
	_config = std::make_shared<Config>(Config::getDefult());
	_resources = std::make_shared<Resources>();

	QSurfaceFormat fmt;
  fmt.setVersion(3, 2);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setDepthBufferSize(24);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	fmt.setSamples(4);
  QSurfaceFormat::setDefaultFormat(fmt);

	Engine engine(argc, argv);
	engine.configure(_config);
	engine.setup(_config, _resources);

	installMessageHandler();

	std::unique_ptr<MainFrame> mainFrame;
	const auto session = From<AppSession>::from(_config->app_session)
		.value_or(AppSession::Test);

	switch (session) {
		case AppSession::Test:
			mainFrame = std::make_unique<MainFrameTest>();
			break;
		case AppSession::ResourcesView:
			mainFrame = std::make_unique<ResourcesViewerFrame>(_resources);
			break;
		default:
			throw ApplicationExceptionUndefinedMainFrame();
	}

	mainFrame->configure(_config);
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
