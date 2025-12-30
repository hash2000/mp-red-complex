#include "Launcher/widgets/profile/profile_view.h"

ProfileView::ProfileView(QWidget* parent)
: QWidget(parent) {
}

QIcon ProfileView::icon() const {
	return QIcon::fromTheme(QIcon::ThemeIcon::UserAvailable);
}

QString ProfileView::description() const {
	return "Current account";
}

QString ProfileView::id() const {
	return "account";
}

ViewState ProfileView::viewState() const {
	return ViewState::UserProfile;
}

void ProfileView::onAction() {
}
