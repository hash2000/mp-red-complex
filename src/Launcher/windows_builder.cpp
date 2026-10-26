#include "Launcher/windows_builder.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/windows/user/login_window.h"
#include "Launcher/windows/user/user_window.h"
#include "Launcher/windows/warmup/warmup_window.h"
#include "Launcher/windows/code_editor/code_editor_window.h"
#include "Launcher/windows/fetch_api/fetch_api_window.h"
#include "Launcher/windows/fetch_api/fetch_api_queries_tree_window.h"

#include <map>
#include <functional>

class WindowsBuilder::Private {
public:
	Private(WindowsBuilder* parent)
		: q(parent) {
	}

	WindowsBuilder* q;
	std::map<QString, std::function<MdiChildWindow*(const QString&, std::shared_ptr<CommandContext> context, QWidget*)>> factory;
};

WindowsBuilder::WindowsBuilder()
: d(std::make_unique<Private>(this)) {
	using CommandContextPtr = std::shared_ptr<CommandContext>;

	d->factory.emplace("user-login", [](const QString& id, CommandContextPtr context, QWidget* parent) { return new LoginWindow(id, context, parent); });
	d->factory.emplace("user-profile", [](const QString& id, CommandContextPtr context, QWidget* parent) { return new UserWindow(id, context, parent); });
	d->factory.emplace("warmup", [](const QString& id, CommandContextPtr context, QWidget* parent) { return new WarmupWindow(id, context, parent); });
	d->factory.emplace("code-editor", [](const QString& id, CommandContextPtr context, QWidget* parent) { return new CodeEditorWindow(id, context, parent); });
	d->factory.emplace("fetch-api", [](const QString& id, CommandContextPtr context, QWidget* parent) { return new FetchApiWindow(id, context, parent); });
	d->factory.emplace("fetch-api-queries-tree", [](const QString& id, CommandContextPtr context, QWidget* parent) { return new FetchApiQueriesTreeWindow(id, context, parent); });
}

WindowsBuilder::~WindowsBuilder() = default;

MdiChildWindow* WindowsBuilder::build(const QString& name, const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent) {
	const auto &it = d->factory.find(name);
	if (it == d->factory.end()) {
		return nullptr;
	}

	return it->second(id, context, parent);
}
