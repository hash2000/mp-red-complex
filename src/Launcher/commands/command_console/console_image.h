#pragma once
#include <QPixmap>
#include <QVariant>
#include <optional>

class ConsoleImage {
public:
	ConsoleImage(const QPixmap& value, int width = -1, int height = -1);
	ConsoleImage(const QVariant& value, int width = -1, int height = -1);

	QString toHtml() const;

private:
	void setPixmap(const QPixmap& value, int width, int height);

private:
	std::optional<QPixmap> _pixmap;
};
