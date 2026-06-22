#include "Launcher/commands/command_console/console_image.h"
#include <QBuffer>
#include <QByteArray>
#include <QIODevice>

ConsoleImage::ConsoleImage(const QPixmap& value, int width, int height) {
	setPixmap(value, width, height);
}

ConsoleImage::ConsoleImage(const QVariant& value, int width, int height) {
	if (value.type() == QVariant::Pixmap) {
		setPixmap(value.value<QPixmap>(), width, height);
	}
}

void ConsoleImage::setPixmap(const QPixmap& value, int width, int height) {
	if (value.isNull()) {
		return;
	}

	// Определяем финальные размеры с сохранением пропорций
	int finalWidth = value.width();
	int finalHeight = value.height();

	// Случай 1: заданы оба размера
	if (width != -1 && height != -1) {
		finalWidth = width;
		finalHeight = height;
	}
	// Случай 2: задана только ширина
	else if (width != -1) {
		finalWidth = width;
		// Вычисляем высоту с сохранением пропорций
		finalHeight = qRound(static_cast<qreal>(value.height()) * width / value.width());
	}
	// Случай 3: задана только высота  
	else if (height != -1) {
		finalHeight = height;
		// Вычисляем ширину с сохранением пропорций
		finalWidth = qRound(static_cast<qreal>(value.width()) * height / value.height());
	}
	// Случай 4: оба -1 → используем исходный размер

	_pixmap = value.scaled(finalWidth, finalHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

