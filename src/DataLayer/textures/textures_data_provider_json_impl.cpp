#include "DataLayer/textures/textures_data_provider_json_impl.h"
#include "DataStream/format/pixmap/data_reader.h"
#include "Resources/resources.h"
#include <QPainter>
#include <array>

class TexturesDataProviderJsonImpl::Private {
public:
	Private(TexturesDataProviderJsonImpl* parent)
		: q(parent) {
	}

	// Таблица маппинга TextureType к директориям
	struct TexturePathMapping {
		TextureType type;
		const char* directory;
	};

	static constexpr std::array<TexturePathMapping, 5> s_textureMappings = {{
		{ TextureType::Icon,      "icons" },
		{ TextureType::Entity,    "entities" },
		{ TextureType::Item,      "items" },
		{ TextureType::Character, "characters" },
		{ TextureType::Equipment, "equipment" },
	}};

	// Получить директорию по типу текстуры
	static const char* getDirectoryForType(TextureType type) {
		for (const auto& mapping : s_textureMappings) {
			if (mapping.type == type) {
				return mapping.directory;
			}
		}
		// По умолчанию - icons
		return "icons";
	}

	// Построить полный путь к текстуре
	static QString buildTexturePath(const QString& relativePath, TextureType type) {
		const char* directory = getDirectoryForType(type);
		return QString("%1/%2").arg(directory, relativePath);
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

std::optional<QPixmap> TexturesDataProviderJsonImpl::loadTexture(const QString& path, TextureType type) const {
	// Строим полный путь на основе типа текстуры
	const auto fullPath = Private::buildTexturePath(path, type);

	QPixmap pixmap;
	Format::Pixmap::DataReader reader(d->resources, "assets", fullPath);
	if (!reader.read(pixmap)) {
		return std::nullopt;
	}
	return pixmap;
}
