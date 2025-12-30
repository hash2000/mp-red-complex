#include "Engine/application.h"
#include "Engine/application/application_session.h"
#include "Engine/application/application_exception.h"
#include "Engine/engine.h"
#include <QSurfaceFormat>


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

	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

	Engine engine(argc, argv);
	engine.configure(_config);
	engine.setup(_config, _resources);

	installMessageHandler();

	const auto session = From<AppSession>::from(_config->app_session)
		.value_or(AppSession::Test);

	auto mainFrame = createMainFrame();
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

std::shared_ptr<Resources> Application::resources() {
	return _resources;
}
