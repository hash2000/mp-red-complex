#include "Libs/Engine/application.h"
#include "Libs/Engine/main_frame.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/resources/model/assets_model.h"
#include "Libs/DataFormat/proto/procedure.h"
#include <QSurfaceFormat>
#include <QApplication>
#include <QStyleHints>
#include <QSettings>
#include <QStandardPaths>
#include <QFile>
#include <QDir>

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
	bool setupAppearance();
	bool setupScriptsSubsystem();
	void installMessageHandler();
	bool installStylesheets();
	QPalette darkPalette();
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

	if (!setupResources()) {
		return 1;
	}

	QSurfaceFormat fmt;
	fmt.setVersion(3, 3);
	fmt.setRenderableType(QSurfaceFormat::OpenGL);
	//fmt.setVersion(2, 1);
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

	QApplication::setApplicationName(q->applicationName());
	QApplication::setOrganizationName(q->organizationName());

	QApplication app(argc, argv);




	//installMessageHandler();

	//auto mainFrame = q->createMainFrame(pRes);
	//mainFrame->configure(pCfg);
	//engine.setupMainFrame(std::move(mainFrame));

	qDebug() << "Startup";
	return app.exec();
}

bool Application::Private::setupResources() {
	if (!resources->load()) {
		return false;
	}

	registerTypes();

	if (!setupScriptsSubsystem()) {
		return false;
	}

	setupAppearance();
	return true;
}

bool Application::Private::setupScriptsSubsystem() {


	return true;
}

bool Application::Private::setupAppearance() {
	QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

	if (!installStylesheets()) {
		return false;
	}

	QApplication::setPalette(darkPalette());

	return true;
}

bool Application::Private::installStylesheets() {
	const auto pathVar = resources->Directories.get("system.path.profile");
	if (!pathVar.has_value()) {
		return false;
	}

	const auto path = pathVar.value().filePath("stylesheets/base.qss");
	if (auto file = QFile{ path }; file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {
		qApp->setStyleSheet(QTextStream{ &file }.readAll());
		return true;
	}

	return false;
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

QPalette Application::Private::darkPalette() {
	const auto button = QColor{ 35, 35, 35 };
	const auto text = QColor{ 207, 207, 207 };
	const auto highlight = QColor{ 62, 112, 205 };

	// Build an initial palette based on the button color
	auto palette = QPalette{ button };

	// Window colors
	palette.setColor(QPalette::Active, QPalette::Window, QColor{ 50, 50, 50 });
	palette.setColor(QPalette::Inactive, QPalette::Window, QColor{ 40, 40, 40 });
	palette.setColor(QPalette::Disabled, QPalette::Window, QColor{ 50, 50, 50 }.darker(200));

	// List box backgrounds, text entry backgrounds, menu backgrounds
	palette.setColor(QPalette::Base, button.darker(130));

	// Button text
	palette.setColor(QPalette::Active, QPalette::ButtonText, text);
	palette.setColor(QPalette::Inactive, QPalette::ButtonText, text);
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, text.darker(200));

	// WindowText is supposed to be against QPalette::Window
	palette.setColor(QPalette::Active, QPalette::WindowText, text);
	palette.setColor(QPalette::Inactive, QPalette::WindowText, text);
	palette.setColor(QPalette::Disabled, QPalette::WindowText, text.darker(200));

	// Menu text, text edit text, table cell text
	palette.setColor(QPalette::Active, QPalette::Text, text.darker(115));
	palette.setColor(QPalette::Inactive, QPalette::Text, text.darker(115));

	// Disabled menu item text color
	palette.setColor(QPalette::Disabled, QPalette::Text, QColor{ 102, 102, 102 });

	// Disabled menu item text shadow
	palette.setColor(QPalette::Disabled, QPalette::Light, button.darker(200));

	// Highlight (selected list box row, selected grid cell background, selected tab text
	palette.setColor(QPalette::Active, QPalette::Highlight, highlight);
	palette.setColor(QPalette::Inactive, QPalette::Highlight, highlight);
	palette.setColor(QPalette::Disabled, QPalette::Highlight, highlight);

	return palette;
}
