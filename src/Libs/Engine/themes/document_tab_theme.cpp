#include "Libs/Engine/themes/document_tab_theme.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <yaml-cpp/yaml.h>

#include <exception>

namespace {

const QString kThemesFileName = QStringLiteral("documents.yaml");

/// Цвет из скалярного узла yaml: `"#61afef"`, `red`, `"#aabbccdd"`.
QColor parseColor(const YAML::Node& node) {
	if (!node || !node.IsScalar()) {
		return QColor();
	}

	const auto text = QString::fromStdString(node.as<std::string>()).trimmed();
	if (text.isEmpty()) {
		return QColor();
	}

	const QColor color(text);
	if (!color.isValid()) {
		qWarning() << "DocumentTabTheme: invalid color value:" << text;
	}

	return color;
}

/// Глиф из скалярного узла yaml: обычно один символ UTF-8.
QString parseGlyph(const YAML::Node& node) {
	if (!node || !node.IsScalar()) {
		return QString();
	}

	return QString::fromStdString(node.as<std::string>()).trimmed();
}

} // namespace

const QString& DocumentTabTheme::fileName() {
	return kThemesFileName;
}

DocumentTabTheme DocumentTabTheme::loadFromThemeDirectory(const QString& themeDir) {
	if (themeDir.isEmpty()) {
		return DocumentTabTheme();
	}

	return loadFromFile(QDir(themeDir).filePath(kThemesFileName));
}

DocumentTabTheme DocumentTabTheme::loadFromFile(const QString& filePath) {
	DocumentTabTheme theme;
	if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
		return theme;
	}

	try {
		const auto root = YAML::LoadFile(filePath.toStdString());
		const auto tabs = root["tabs"];
		if (!tabs || !tabs.IsMap()) {
			qWarning() << "DocumentTabTheme: no 'tabs' section in" << filePath;
			return theme;
		}

		theme._defaultColor = parseColor(tabs["default"]);
		theme._defaultGlyph = parseGlyph(tabs["defaultGlyph"]);

		const auto colors = tabs["colors"];
		if (colors && colors.IsMap()) {
			for (const auto& entry : colors) {
				const auto key = QString::fromStdString(entry.first.as<std::string>()).trimmed();
				const auto color = parseColor(entry.second);
				if (key.isEmpty() || !color.isValid()) {
					qWarning() << "DocumentTabTheme: skip invalid color entry in" << filePath;
					continue;
				}

				theme._colors.insert(key, color);
			}
		}

		const auto glyphs = tabs["glyphs"];
		if (glyphs && glyphs.IsMap()) {
			for (const auto& entry : glyphs) {
				const auto key = QString::fromStdString(entry.first.as<std::string>()).trimmed();
				const auto glyph = parseGlyph(entry.second);
				if (key.isEmpty() || glyph.isEmpty()) {
					qWarning() << "DocumentTabTheme: skip empty glyph entry in" << filePath;
					continue;
				}

				theme._glyphs.insert(key, glyph);
			}
		}
	}
	catch (const std::exception& ex) {
		qWarning() << "DocumentTabTheme: failed to load" << filePath << ex.what();
		theme.clear();
	}

	return theme;
}

QColor DocumentTabTheme::colorFor(const QString& className, const QString& xtype) const {
	// Полное имя класса точнее: у класса может быть несколько xtype
	// (yaml-объявления с разными псевдонимами).
	if (!className.isEmpty()) {
		const auto byClass = _colors.constFind(className);
		if (byClass != _colors.constEnd()) {
			return byClass.value();
		}
	}

	if (!xtype.isEmpty()) {
		const auto byXtype = _colors.constFind(xtype);
		if (byXtype != _colors.constEnd()) {
			return byXtype.value();
		}
	}

	return _defaultColor;
}

QString DocumentTabTheme::glyphFor(const QString& className, const QString& xtype) const {
	if (!className.isEmpty()) {
		const auto byClass = _glyphs.constFind(className);
		if (byClass != _glyphs.constEnd()) {
			return byClass.value();
		}
	}

	if (!xtype.isEmpty()) {
		const auto byXtype = _glyphs.constFind(xtype);
		if (byXtype != _glyphs.constEnd()) {
			return byXtype.value();
		}
	}

	return _defaultGlyph;
}

QColor DocumentTabTheme::defaultColor() const {
	return _defaultColor;
}

QString DocumentTabTheme::defaultGlyph() const {
	return _defaultGlyph;
}

QHash<QString, QColor> DocumentTabTheme::colors() const {
	return _colors;
}

QHash<QString, QString> DocumentTabTheme::glyphs() const {
	return _glyphs;
}

bool DocumentTabTheme::isEmpty() const {
	return _colors.isEmpty() && _glyphs.isEmpty() &&
		!_defaultColor.isValid() && _defaultGlyph.isEmpty();
}

void DocumentTabTheme::clear() {
	_defaultColor = QColor();
	_defaultGlyph.clear();
	_colors.clear();
	_glyphs.clear();
}
