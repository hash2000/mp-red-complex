#include "DataLayer/textures/textures_data_provider_json_impl.h"
#include "DataStream/format/pixmap/data_reader.h"
#include "Resources/resources.h"
#include <QPainter>

class TexturesDataProviderJsonImpl::Private {
public:
	Private(TexturesDataProviderJsonImpl* parent)
		: q(parent) {
	}

	QPixmap loadEmptyStubIcon(const QString& name) {
		QPixmap result = QPixmap(64, 64);
		result.fill(Qt::darkGray);
		QPainter painter(&result);
		painter.setPen(Qt::white);
		painter.drawText(result.rect(), Qt::AlignCenter, name.left(2));
		return result;
	}

	TexturesDataProviderJsonImpl* q;
	Resources* resources;
};

TexturesDataProviderJsonImpl::TexturesDataProviderJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

TexturesDataProviderJsonImpl::~TexturesDataProviderJsonImpl() = default;

std::optional<QPixmap> TexturesDataProviderJsonImpl::loadTexture(const QString& path) const {
	QPixmap pixmap;
	Format::Pixmap::DataReader reader(d->resources, "assets", path);
	if (!reader.read(pixmap)) {
		return std::nullopt;
	}
	return pixmap;
}

std::optional<QPixmap> TexturesDataProviderJsonImpl::loadIcon(const QString& iconName) const {
	// Путь к иконке в папке icons
	const auto path = QString("icons/%1").arg(iconName);

	QPixmap pixmap;
	Format::Pixmap::DataReader reader(d->resources, "assets", path);
	if (!reader.read(pixmap)) {
		// Возвращаем заглушку, если иконка не найдена
		return d->loadEmptyStubIcon(iconName);
	}

	return pixmap;
}
