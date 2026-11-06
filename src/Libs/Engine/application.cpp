#include "Libs/Engine/application.h"
#include "Libs/Engine/main_frame.h"
#include "Libs/Engine/bootstrap/bootstrap_manager.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/resources/model/assets_model.h"
#include "Libs/DataFormat/proto/procedure.h"
#include <QSurfaceFormat>
#include <QApplication>
#include <QStyleHints>
#include <QSettings>

class Application::Private {
public:
	Private(Application* parent) : q(parent) { }
	Application* q;
	std::unique_ptr<Resources> resources;

public:
	int tryRun(int& argc, char** argv);

private:
	void registerTypes();
	bool setupResources();
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

	QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication::setAttribute(Qt::AA_ShareOpenGLContexts);  // Общий OpenGL контекст для всех виджетов
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
	QGuiApplication::setAttribute(Qt::AA_UseStyleSheetPropagationInWidgetStyles);
	QGuiApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
	QSettings::setDefaultFormat(QSettings::IniFormat);

	qputenv("QT_MAC_DONT_OVERRIDE_CTRL_LMB", "1");
	qputenv("QT_OPENGL_BUGLIST", ":/opengl_buglist.json");

	QApplication app(argc, argv);

	if (!setupResources()) {
		return 1;
	}


	//installMessageHandler();

	//auto mainFrame = q->createMainFrame(pRes);
	//mainFrame->configure(pCfg);
	//engine.setupMainFrame(std::move(mainFrame));

	qDebug() << "Start application";
	return QApplication::exec();
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
	qRegisterMetaType<AssetsViewItemType>("AssetsViewItemType");
	qRegisterMetaType<ProgramOpCode>("OpCode");
}
