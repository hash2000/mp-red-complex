#include "Content/CodeEditorWidget/highlights/theme_highlighter_plugin.h"

#include <yaml-cpp/yaml.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QStringList>

namespace {

/// Читает стиль текста из узла правила: цвета, начертание, размер, шрифт.
QTextCharFormat parseFormat(const YAML::Node& node) {
	QTextCharFormat format;

	if (!node || !node.IsMap()) {
		return format;
	}

	const auto foreground = node["foreground"];
	if (foreground && foreground.IsScalar()) {
		format.setForeground(QColor(QString::fromStdString(foreground.Scalar())));
	}

	const auto background = node["background"];
	if (background && background.IsScalar()) {
		format.setBackground(QColor(QString::fromStdString(background.Scalar())));
	}

	if (node["bold"] && node["bold"].as<bool>(false)) {
		format.setFontWeight(QFont::Bold);
	}

	if (node["italic"] && node["italic"].as<bool>(false)) {
		format.setFontItalic(true);
	}

	if (node["underline"] && node["underline"].as<bool>(false)) {
		format.setFontUnderline(true);
	}

	if (node["strikeout"] && node["strikeout"].as<bool>(false)) {
		format.setFontStrikeOut(true);
	}

	const auto pointSize = node["pointSize"];
	if (pointSize && pointSize.IsScalar()) {
		format.setFontPointSize(pointSize.as<double>());
	}

	const auto family = node["fontFamily"];
	if (family && family.IsScalar()) {
		format.setFontFamilies({ QString::fromStdString(family.Scalar()) });
	}

	return format;
}

/// Опции регулярного выражения: `options: [caseInsensitive]`.
QRegularExpression::PatternOptions parseOptions(const YAML::Node& node) {
	QRegularExpression::PatternOptions options;
	const auto list = node["options"];
	if (!list || !list.IsSequence()) {
		return options;
	}

	for (const auto& item : list) {
		const auto name = QString::fromStdString(item.as<std::string>()).toLower();
		if (name == "caseinsensitive") {
			options |= QRegularExpression::CaseInsensitiveOption;
		}
		else if (name == "multiline") {
			options |= QRegularExpression::MultilineOption;
		}
		else if (name == "dotmatcheseverything") {
			options |= QRegularExpression::DotMatchesEverythingOption;
		}
		else {
			qWarning() << "ThemeHighlighterPlugin: unknown regex option:" << name;
		}
	}

	return options;
}

int parsePriority(const YAML::Node& node) {
	const auto priority = node["priority"];
	return (priority && priority.IsScalar()) ? priority.as<int>() : 0;
}

QRegularExpression parsePattern(const QString& pattern, const QRegularExpression::PatternOptions& options) {
	QRegularExpression expression(pattern, options);
	if (!expression.isValid()) {
		qWarning() << "ThemeHighlighterPlugin: invalid pattern:" << pattern
			<< expression.errorString();
	}

	return expression;
}

} // namespace

ThemeHighlighterPlugin::ThemeHighlighterPlugin(const QString& themeFile) {
	const auto filePath = themeFile;
	QFileInfo info(filePath);
	if (!info.exists()) {
		qWarning() << "ThemeHighlighterPlugin: file is not found:" << filePath;
		return;
	}

	YAML::Node root;
	try {
		root = YAML::LoadFile(filePath.toStdString());
	}
	catch (const std::exception& ex) {
		qWarning() << "ThemeHighlighterPlugin: cannot parse" << filePath << ex.what();
		return;
	}

	const auto language = root["language"];
	if (!language || !language.IsMap()) {
		qWarning() << "ThemeHighlighterPlugin: no language block in" << filePath;
		return;
	}

	_info.name = QString::fromStdString(language["name"].as<std::string>(""));
	_info.mimeType = QString::fromStdString(language["mimeType"].as<std::string>(""));

	const auto extensions = language["extensions"];
	if (extensions && extensions.IsSequence()) {
		for (const auto& item : extensions) {
			_info.extensions << QString::fromStdString(item.as<std::string>());
		}
	}

	const auto icon = language["icon"];
	if (icon && icon.IsScalar()) {
		_info.icon = QIcon(QString::fromStdString(icon.Scalar()));
	}

	if (_info.name.isEmpty() || _info.mimeType.isEmpty()) {
		qWarning() << "ThemeHighlighterPlugin: language name and mimeType are required in" << filePath;
	}

	const auto rules = root["rules"];
	if (rules && rules.IsSequence()) {
		for (const auto& node : rules) {
			const auto pattern = node["pattern"];
			if (!pattern || !pattern.IsScalar()) {
				qWarning() << "ThemeHighlighterPlugin: rule without pattern in" << filePath;
				continue;
			}

			RuleSpec spec;
			spec.pattern = parsePattern(QString::fromStdString(pattern.Scalar()), parseOptions(node));
			spec.format = parseFormat(node);
			spec.priority = parsePriority(node);
			_rules.push_back(spec);
		}
	}

	const auto embedded = root["embeddedBlocks"];
	if (embedded && embedded.IsSequence()) {
		for (const auto& node : embedded) {
			const auto start = node["start"];
			const auto end = node["end"];
			if (!start || !start.IsScalar() || !end || !end.IsScalar()) {
				qWarning() << "ThemeHighlighterPlugin: embedded block needs start and end in" << filePath;
				continue;
			}

			EmbeddedSpec spec;
			spec.start = parsePattern(QString::fromStdString(start.Scalar()), parseOptions(node));
			spec.end = parsePattern(QString::fromStdString(end.Scalar()), parseOptions(node));
			spec.language = QString::fromStdString(node["language"].as<std::string>(""));
			spec.format = parseFormat(node);
			spec.priority = parsePriority(node);
			_embeddedRules.push_back(spec);
		}
	}
}

ThemeHighlighterPlugin::~ThemeHighlighterPlugin() = default;

LanguageInfo ThemeHighlighterPlugin::languageInfo() const {
	return _info;
}

void ThemeHighlighterPlugin::install() {
	if (_installed) {
		return;
	}

	_installed = true;

	for (const auto& rule : _rules) {
		addRule(Rule{ rule.pattern, rule.format, rule.priority });
	}

	for (const auto& rule : _embeddedRules) {
		addEmbeddedBlockRule(EmbeddedBlockRule{
			rule.start,
			rule.end,
			rule.language,
			rule.format,
			rule.priority });
	}
}

std::vector<std::unique_ptr<HighlightingPlugin>> ThemeHighlighterPlugin::loadDirectory(const QString& directory) {
	std::vector<std::unique_ptr<HighlightingPlugin>> plugins;

	QDir dir(directory);
	if (!dir.exists()) {
		qWarning() << "ThemeHighlighterPlugin: highlighters directory is not found:" << directory;
		return plugins;
	}

	const auto files = dir.entryInfoList({ QStringLiteral("*.yaml") }, QDir::Files, QDir::Name);
	for (const auto& file : files) {
		auto plugin = std::make_unique<ThemeHighlighterPlugin>(file.absoluteFilePath());
		if (plugin->languageInfo().mimeType.isEmpty()) {
			continue;
		}

		// Правила готовы к использованию сразу после загрузки каталога.
		plugin->install();
		plugins.push_back(std::move(plugin));
	}

	return plugins;
}
