#include "Libs/Graphics/textures/extensions/pixmap_extensions.h"

namespace Extensions::Pixmaps {
QPixmap scale(const QPixmap& src, int width, int height) {
	if (src.isNull()) {
		return src;
	}

	// Определяем финальные размеры с сохранением пропорций
	int finalWidth = src.width();
	int finalHeight = src.height();

	// Случай 1: заданы оба размера
	if (width != -1 && height != -1) {
		finalWidth = width;
		finalHeight = height;
	}
	// Случай 2: задана только ширина
	else if (width != -1) {
		finalWidth = width;
		// Вычисляем высоту с сохранением пропорций
		finalHeight = qRound(static_cast<qreal>(src.height()) * width / src.width());
	}
	// Случай 3: задана только высота  
	else if (height != -1) {
		finalHeight = height;
		// Вычисляем ширину с сохранением пропорций
		finalWidth = qRound(static_cast<qreal>(src.width()) * height / src.height());
	}
	// Случай 4: оба -1 → используем исходный размер

	return src.scaled(finalWidth, finalHeight,
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation);
}
}
