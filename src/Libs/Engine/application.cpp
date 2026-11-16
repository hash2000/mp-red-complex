#include "Libs/Engine/application.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_manager.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_context_builder.h"
#include "Libs/Engine/scripting/application/class_manager.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Resources/resources.h"
#include <QSurfaceFormat>
#include <QApplication>
#include <QStyleHints>
#include <QSettings>
#include <QWidget>

class Application::Private {
public:
	Private(Application* parent) : q(parent) { }
	Application* q;
	std::unique_ptr<Resources> resources;
	std::unique_ptr<BootstrapContextBuilder> contextBuilder;
	BootstrapConfig::Config bootstrapConfig;

public:
	int tryRun(int& argc, char** argv);

private:
	void setupQt();
	bool setupResources();
	CommandController* initializeCommandController();
	std::unique_ptr<QWidget> createViewport(CommandController* commandController);
	void registerTypes();
	void installMessageHandler();
};

Application::Application()
	: d(std::make_unique<Private>(this)) {
}

Application::~Application() = default;


int Application::run(int &argc, char **argv) {
	try {
		return d->tryRun(argc, argv);
	}
	catch(std::exception &ex) {
		qFatal() << "Application fatal exception: " << ex.what();
	}

	return 0;
}

int Application::Private::tryRun(int& argc, char** argv) {
	qDebug() << "Bootstrap" << q->applicationName();
	resources = std::make_unique<Resources>();
	setupQt();

	QApplication app(argc, argv);

	if (!setupResources()) {
		return 1;
	}

	auto commandController = initializeCommandController();
	if (!commandController) {
		return 1;
	}

	auto viewport = createViewport(commandController);
	if (!viewport) {
		qWarning() << "Declarative viewport is not available.";
		return -1;
	}

	viewport->resize(1400, 900);
	viewport->show();

	qDebug() << "Start application";
	return QApplication::exec();
}

void Application::Private::setupQt() {
	QApplication::setApplicationName(q->applicationName());
	QApplication::setOrganizationName(q->organizationName());

	QSurfaceFormat fmt;
	//fmt.setVersion(2, 1);
	fmt.setVersion(3, 3);
	fmt.setRenderableType(QSurfaceFormat::OpenGL);
	fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
	fmt.setDepthBufferSize(24);
	fmt.setAlphaBufferSize(0);
	fmt.setSamples(4);
	fmt.setProfile(QSurfaceFormat::CoreProfile);
	fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
	fmt.setSamples(4);
	fmt.setStencilBufferSize(8);
	//fmt.setStereo(true);

	QSurfaceFormat::setDefaultFormat(fmt);

	QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

	//QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);  // Общий OpenGL контекст для всех виджетов
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
	QGuiApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);
	QGuiApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
	QSettings::setDefaultFormat(QSettings::IniFormat);

	qDebug() << "Plugin path:" << qgetenv("QT_PLUGIN_PATH");
	qDebug() << "Current platform:" << qgetenv("QT_QPA_PLATFORM");
}

CommandController* Application::Private::initializeCommandController() {
	auto commandController = q->buildCommandController(resources.get());
	if (!commandController) {
		qWarning() << "Command controller is not available.";
		return nullptr;
	}

	contextBuilder = std::make_unique<BootstrapContextBuilder>(commandController);
	contextBuilder->reload(bootstrapConfig);
	commandController->init();
	return commandController;
}

std::unique_ptr<QWidget> Application::Private::createViewport(CommandController* commandController) {
	ClassManager classManager(resources.get(), commandController);
	return std::unique_ptr<QWidget>(classManager.createViewport(bootstrapConfig.application));
}

bool Application::Private::setupResources() {
	if (!resources->load()) {
		return false;
	}

	registerTypes();

	BootstrapManager bootstrap(resources.get());
	if (!bootstrap.reload()) {
		return false;
	}

	bootstrapConfig = bootstrap.config();
	return true;
}

void Application::Private::installMessageHandler() {
	static QtMessageHandler originalMessageHandler = nullptr;
	originalMessageHandler = qInstallMessageHandler([](QtMsgType type,
		const QMessageLogContext& context,
		const QString& message) {
		if (originalMessageHandler) {
			originalMessageHandler(type, context, message);
		}

		const auto msg = qFormatLogMessage(type, context, message);

	});
}

void Application::Private::registerTypes() {

}
