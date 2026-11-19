#include "Content/UsersModule/documents/login_document.h"
#include "Content/UsersModule/widgets/login_widget.h"

LoginDocument::LoginDocument(UsersService* usersService, QWidget* parent)
	: DocumentWidgetStub(new LoginWidget(usersService), parent) {
	setGlyph(QStringLiteral("🔑"));
	setTitle(tr("Вход"));
}
