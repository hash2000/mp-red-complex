#include "DataLayer/images/images_data_provider_json_impl.h"
#include "DataStream/format/pixmap/data_reader.h"
#include "Resources/resources.h"
#include <QPainter>
#include <array>

class ImagesDataProviderJsonImpl::Private {
public:
	Private(ImagesDataProviderJsonImpl* parent)
		: q(parent) {
	}

	// Таблица маппинга ImageType к директориям
	struct TexturePathMapping {
		ImageType type;
		const char* directory;
	};

	static constexpr std::array<TexturePathMapping, static_cast<size_t>(ImageType::LastSlot)> s_textureMappings = {{
		{ ImageType::Icon,      "icons" },
		{ ImageType::Entity,    "entities" },
		{ ImageType::Item,      "items" },
		{ ImageType::Character, "characters" },
		{ ImageType::Users,			"users" },
		{ ImageType::TileSets,	"textures/tiles" },
		{ ImageType::Equipment, "equipment" },
	}};

	// Получить директорию по типу текстуры
	static const char* getDirectoryForType(ImageType type) {
		for (const auto& mapping : s_textureMappings) {
			if (mapping.type == type) {
				return mapping.directory;
			}
		}
		// По умолчанию - icons
		return "icons";
	}

	// Построить полный путь к текстуре
	static QString buildTexturePath(const QString& relativePath, ImageType type) {
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

	ImagesDataProviderJsonImpl* q;
	Resources* resources;
};

ImagesDataProviderJsonImpl::ImagesDataProviderJsonImpl(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

ImagesDataProviderJsonImpl::~ImagesDataProviderJsonImpl() = default;

std::optional<QPixmap> ImagesDataProviderJsonImpl::load(const QString& path, ImageType type) const {
	// Строим полный путь на основе типа текстуры
	const auto fullPath = Private::buildTexturePath(path, type);

	QPixmap pixmap;
	Format::Pixmap::DataReader reader(d->resources, "assets", fullPath);
	if (!reader.read(pixmap)) {
		return std::nullopt;
	}
	return pixmap;
}

QStringList ImagesDataProviderJsonImpl::list(ImageType type) const {
	QStringList result;
	const char* directory = Private::getDirectoryForType(type);
	const QString prefix = QString("%1/").arg(directory);

	// Получаем все потоки из ресурсов
	for (const auto& container : d->resources->items()) {
		const auto items = container.items();
		for (const auto& [path, stream] : items) {
			// Проверяем, что путь начинается с нужной директории и имеет расширение .png
			if (stream->containerName() == "assets" && path.startsWith(prefix) && path.endsWith(".png", Qt::CaseInsensitive)) {
				// Извлекаем только имя файла
				const QString fileName = path.mid(prefix.length());
				result.append(fileName);
			}
		}
	}

	result.sort();
	return result;
}

