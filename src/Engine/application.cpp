#include "Engine/application.h"
#include "Engine/engine.h"
#include "Engine/main_frame.h"
#include "Base/config.h"
#include "Resources/resources.h"
#include <QSurfaceFormat>
#include <QApplication>

class Application::Private {
public:
	Private(Application* parent)
		: q(parent) {
	}

	Application* q;
	std::shared_ptr<Config> config;
	std::unique_ptr<Resources> resources;
};

Application::Application()
	: d(std::make_unique<Private>(this)) {
}

Application::~Application() = default;


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
	d->config = std::make_shared<Config>(Config::getDefult());
	d->resources = std::make_unique<Resources>();

	QSurfaceFormat fmt;
  fmt.setVersion(3, 3);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setDepthBufferSize(24);
  fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	fmt.setSamples(4);
	fmt.setStencilBufferSize(8);
	fmt.setStereo(true);

  QSurfaceFormat::setDefaultFormat(fmt);

	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

	Engine engine(argc, argv);
	engine.configure(d->config);
	engine.setup(d->config, resources());

	installMessageHandler();

	auto mainFrame = createMainFrame();
	mainFrame->configure(d->config);
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

Resources* Application::resources() {
	return d->resources.get();
}
