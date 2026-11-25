#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/highlights/theme_highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/language_info.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

namespace {

QString themeHighlightersDir() {
	return QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR) + QStringLiteral("/assets/themes/dark/highlighters");
}

/// Ищет плагин по mime-типу среди загруженных из темы.
HighlightingPlugin* findPlugin(const std::vector<std::unique_ptr<HighlightingPlugin>>& plugins,
	const QString& mimeType) {
	for (const auto& plugin : plugins) {
		if (plugin->languageInfo().mimeType == mimeType) {
			return plugin.get();
		}
	}

	return nullptr;
}

int countFormatted(const std::vector<HighlightingPlugin::HighlightingMatch>& matches) {
	return static_cast<int>(matches.size());
}

} // namespace

class TestHighlighters : public QObject {
	Q_OBJECT

private slots:
	void themeDirectoryIsLoaded();
	void jsonRulesComeFromTheme();
	void markdownEmbeddedBlockIsParsed();
	void pluginWithoutRulesIsStillLoaded();
	void missingDirectoryLoadsNothing();
};

void TestHighlighters::themeDirectoryIsLoaded() {
	const auto dir = themeHighlightersDir();
	QVERIFY2(QDir(dir).exists(), qPrintable(dir));

	auto plugins = ThemeHighlighterPlugin::loadDirectory(dir);
	QCOMPARE(static_cast<int>(plugins.size()), 7);

	for (const auto& plugin : plugins) {
		const auto info = plugin->languageInfo();
		QVERIFY(!info.mimeType.isEmpty());
		QVERIFY(!info.name.isEmpty());
		QVERIFY(!info.extensions.isEmpty());
	}

	QVERIFY(findPlugin(plugins, QStringLiteral("application/json")));
	QVERIFY(findPlugin(plugins, QStringLiteral("text/x-sh")));
	QVERIFY(findPlugin(plugins, QStringLiteral("text/x-glsl")));
	QVERIFY(findPlugin(plugins, QStringLiteral("text/plane")));
}

void TestHighlighters::jsonRulesComeFromTheme() {
	auto plugins = ThemeHighlighterPlugin::loadDirectory(themeHighlightersDir());
	auto* json = findPlugin(plugins, QStringLiteral("application/json"));
	QVERIFY(json);

	// Ключ объекта: правило с приоритетом 10 из json.yaml.
	const auto keyMatches = json->matches(QStringLiteral(R"("name": "value")"));
	QVERIFY(!keyMatches.empty());

	// Комментарий // — правило из темы, а не из кода.
	const auto commentMatches = json->matches(QStringLiteral("// comment"));
	QVERIFY(!commentMatches.empty());

	// Цвет ключа берётся из темы.
	bool hasExpectedColor = false;
	for (const auto& match : keyMatches) {
		if (match.format.foreground().color() == QColor(QStringLiteral("#CE9178"))) {
			hasExpectedColor = true;
		}
	}
	QVERIFY(hasExpectedColor);
}

void TestHighlighters::markdownEmbeddedBlockIsParsed() {
	auto plugins = ThemeHighlighterPlugin::loadDirectory(themeHighlightersDir());
	auto* markdown = findPlugin(plugins, QStringLiteral("text/markdown"));
	QVERIFY(markdown);

	const auto startMatches = markdown->matchesEmbeddedBlock(QStringLiteral("```cpp"));
	QVERIFY(!startMatches.empty());

	bool languageResolved = false;
	for (const auto& match : startMatches) {
		if (match.language == QStringLiteral("cpp")) {
			languageResolved = true;
		}
	}
	// Язык извлекается из первой группы захвата начала блока.
	QVERIFY(languageResolved);
}

void TestHighlighters::pluginWithoutRulesIsStillLoaded() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	QFile file(dir.filePath(QStringLiteral("custom.yaml")));
	QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
	file.write(R"(language:
  name: Custom
  mimeType: text/x-custom
  extensions: [.custom]
)");
	file.close();

	auto plugins = ThemeHighlighterPlugin::loadDirectory(dir.path());
	QCOMPARE(static_cast<int>(plugins.size()), 1);
	QCOMPARE(plugins.front()->languageInfo().mimeType, QStringLiteral("text/x-custom"));
	QVERIFY(plugins.front()->matches(QStringLiteral("anything")).empty());
}

void TestHighlighters::missingDirectoryLoadsNothing() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	auto plugins = ThemeHighlighterPlugin::loadDirectory(dir.filePath(QStringLiteral("absent")));
	QVERIFY(plugins.empty());
}

QTEST_MAIN(TestHighlighters)
#include "test_highlighters.moc"
