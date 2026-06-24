#include "Launcher/commands/command_console/console_image.h"
#include "Libs/Graphics/textures/extensions/pixmap_extensions.h"
#include <QBuffer>
#include <QByteArray>
#include <QIODevice>

ConsoleImage::ConsoleImage(const QPixmap& value, int width, int height) {
	_pixmap = Extensions::Pixmaps::scale(value, width, height);
}

ConsoleImage::ConsoleImage(const QVariant& value, int width, int height) {
	if (value.type() == QVariant::Pixmap) {
		_pixmap = Extensions::Pixmaps::scale(value.value<QPixmap>(), width, height);
	}
}

QString ConsoleImage::toHtml() const {
	if (!_pixmap.has_value()) {
		return QString();
	}

	QByteArray byteArray;
	QBuffer buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);
	_pixmap->save(&buffer, "PNG");

	const auto base64 = byteArray.toBase64();
	const auto result = QString("<img src=\"data:image/png;base64,%1\" alt=\"image\" />")
		.arg(QString::fromLatin1(base64));

	return result;
}

