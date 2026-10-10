#include "Launcher/windows/warmup/warmup_window.h"
#include "Launcher/windows/warmup/warmup_widget.h"
#include "Content/ConsoleModule/command_context.h"
#include <QTimer>

class WarmupWindow::Private {
public:
	Private(WarmupWindow* parent) : q(parent) {}
	WarmupWindow* q;
	WarmupWidget* widget = nullptr;
	
};

namespace {
	static bool s_allreadyWarmedUp = false;
}

WarmupWindow::WarmupWindow(const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent)
	: MdiChildWindow(id, context, parent)
	, d(std::make_unique<Private>(this)) {	
	d->widget = new WarmupWidget(this);
	setWidget(d->widget);
	setWindowTitle("OpenGL Warmup");
}

WarmupWindow::~WarmupWindow() = default;

bool WarmupWindow::handleCommand(const std::shared_ptr<Instruction> cmd) {
	if (s_allreadyWarmedUp) {
		return false;
	}

	// Через 300мс закрываем родительское окно
	QTimer::singleShot(300, this, [this]() {
		s_allreadyWarmedUp = true;
		close();
	});

	return true;
}
