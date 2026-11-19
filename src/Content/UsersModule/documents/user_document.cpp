#include "Content/UsersModule/documents/user_document.h"
#include "Content/UsersModule/widgets/user_widget.h"

UserDocument::UserDocument(UsersService* usersService, QWidget* parent)
	: DocumentWidgetStub(new UserWidget(usersService), parent) {
	setGlyph(QStringLiteral("👤"));
	setTitle(tr("Профиль"));
}
