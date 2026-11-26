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
#include <QAbstractNativeEventFilter>
#include <QColor>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QScreen>
#include <QFont>
#include <QSettings>
#include <QTimer>
#include <QWidget>

#include <memory>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

#ifdef Q_OS_WIN
/// Windows до первого кадра Qt очищает клиентскую область окна системной
/// белой кистью. Перехватываем WM_ERASEBKGND и заливаем её цветом темы,
/// поэтому даже самый первый кадр не бывает белым.
class ThemedEraseFilter final : public QAbstractNativeEventFilter {
public:
	ThemedEraseFilter(WId window, const QColor& color)
		: window(window), color(color) {
	}

	bool nativeEventFilter(const QByteArray&, void* message, qintptr*) override {
		auto* msg = static_cast<MSG*>(message);
		if (msg->message != WM_ERASEBKGND
			|| msg->hwnd != reinterpret_cast<HWND>(window)) {
			return false;
		}

		HDC dc = reinterpret_cast<HDC>(msg->wParam);
		RECT rect{};
		if (GetClientRect(msg->hwnd, &rect)) {
			HBRUSH brush = CreateSolidBrush(RGB(color.red(), color.green(), color.blue()));
			FillRect(dc, &rect, brush);
			DeleteObject(brush);
		}
		return true;
	}

private:
	WId window = 0;
	QColor color;
};
#endif

} // namespace

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
	void setupAppearance();
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
	setupAppearance();

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
	// Тема уже применена к qApp, но нативное окно Windows до первого кадра
	// успевает показать системный белый фон. Перехватываем стирание фона и
	// заливаем его цветом темы — белая вспышка не появляется вообще.
#ifdef Q_OS_WIN
	auto eraseFilter = std::make_unique<ThemedEraseFilter>(
		viewport->winId(), QApplication::palette().color(QPalette::Window));
	qApp->installNativeEventFilter(eraseFilter.get());
	// После первого кадра Qt сам рисует фон; фильтр нужен только на старте.
	QTimer::singleShot(1000, viewport.get(), [filter = eraseFilter.get()] {
		qApp->removeNativeEventFilter(filter);
	});
#endif
	viewport->show();

	qDebug() << "Start application";
	const int result = QApplication::exec();
#ifdef Q_OS_WIN
	qApp->removeNativeEventFilter(eraseFilter.get());
#endif
	return result;
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

void Application::Private::setupAppearance() {
	// Стиль Fusion выбран намеренно: windows11/windowsvista рисуют кнопки,
	// вкладки и рамки по системной теме Windows (светлая тема, версия ОС,
	// высокий контраст), поэтому на разных машинах одинаковый QSS выглядел
	// по-разному. Fusion использует только палитру и QSS темы.
	if (auto* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) {
		QApplication::setStyle(fusion);
	}
	else {
		qWarning() << "Qt style Fusion is not available, platform default is used:"
			<< QApplication::style()->objectName();
	}

	// Шрифт задаётся явно: иначе системная настройка «Размер текста» и
	// подстановка другого шрифта меняют метрики текста и размеры кнопок.
	QFont applicationFont(QStringLiteral("Segoe UI"));
	applicationFont.setPointSizeF(9.0);
	QApplication::setFont(applicationFont);

	const auto* screen = QGuiApplication::primaryScreen();
	qInfo().nospace()
		<< "Appearance: style=" << QApplication::style()->objectName()
		<< " font=" << QApplication::font().family()
		<< " size=" << QApplication::font().pointSizeF()
		<< " dpr=" << (screen ? screen->devicePixelRatio() : 0.0)
		<< " scaleFactor=" << qgetenv("QT_SCALE_FACTOR");
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
	return std::unique_ptr<QWidget>(classManager.createViewport(bootstrapConfig.appearance.application));
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
