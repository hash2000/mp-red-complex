#include "Content/UsersModule/documents/register_document.h"
#include "Content/UsersModule/widgets/register_widget.h"

RegisterDocument::RegisterDocument(UsersRegistryService* usersRegistryService, QWidget* parent)
	: DocumentWidgetStub(new RegisterWidget(usersRegistryService), parent) {
	setGlyph(QStringLiteral("📝"));
	setTitle(tr("Регистрация"));
}
