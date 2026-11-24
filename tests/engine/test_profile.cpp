#include "test_command.h"
#include "Libs/Engine/profiles/profile.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_manager.h"
#include "Libs/Resources/resources.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

namespace {

BootstrapConfig::Appearance defaultAppearance() {
	BootstrapConfig::Appearance appearance;
	appearance.themeName = QStringLiteral("dark");
	appearance.application.name = QStringLiteral("base");
	appearance.application.viewport = QStringLiteral("BaseViewport");
	return appearance;
}

bool writeFile(const QString& path, const QByteArray& contents) {
	QFile file(path);
	return file.open(QIODevice::WriteOnly | QIODevice::Text)
		&& file.write(contents) == contents.size();
}

} // namespace

void TestEngine::testProfile_EmptyProfileInitializedFromDefaults() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	Profile profile(QStringLiteral("default"), QDir(dir.path()).filePath(QStringLiteral("default")));
	const auto defaults = defaultAppearance();

	QVERIFY(profile.load(defaults));

	// Каталог и файл настроек созданы при первом открытии профиля.
	QVERIFY(QFile::exists(profile.settingsPath()));
	QCOMPARE(profile.name(), QStringLiteral("default"));
	QCOMPARE(profile.appearance().themeName, defaults.themeName);
	QCOMPARE(profile.appearance().application.name, defaults.application.name);
	QCOMPARE(profile.appearance().application.viewport, defaults.application.viewport);
	QVERIFY(profile.isDefault());
	QVERIFY(!profile.allowsEncryptedStores());

	QFile file(profile.settingsPath());
	QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
	const auto contents = file.readAll();
	QVERIFY(contents.contains("theme: dark"));
	QVERIFY(contents.contains("name: base"));
	QVERIFY(contents.contains("xtype: BaseViewport"));
}

void TestEngine::testProfile_SettingsOverrideDefaults() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto profileDir = QDir(dir.path()).filePath(QStringLiteral("default"));
	QVERIFY(QDir().mkpath(profileDir));
	QVERIFY(writeFile(QDir(profileDir).filePath(QStringLiteral("appearance.yaml")), R"(theme: light
application:
  name: singletone
  viewport:
    xtype: SingletoneViewport
)"));

	Profile profile(QStringLiteral("default"), profileDir);
	QVERIFY(profile.load(defaultAppearance()));

	QCOMPARE(profile.appearance().themeName, QStringLiteral("light"));
	QCOMPARE(profile.appearance().application.name, QStringLiteral("singletone"));
	QCOMPARE(profile.appearance().application.viewport, QStringLiteral("SingletoneViewport"));
}

void TestEngine::testProfile_PartialSettingsFallback() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto profileDir = QDir(dir.path()).filePath(QStringLiteral("default"));
	QVERIFY(QDir().mkpath(profileDir));
	QVERIFY(writeFile(QDir(profileDir).filePath(QStringLiteral("appearance.yaml")),
		"theme: light\n"));

	Profile profile(QStringLiteral("default"), profileDir);
	const auto defaults = defaultAppearance();
	QVERIFY(profile.load(defaults));

	QCOMPARE(profile.appearance().themeName, QStringLiteral("light"));
	QCOMPARE(profile.appearance().application.name, defaults.application.name);
	QCOMPARE(profile.appearance().application.viewport, defaults.application.viewport);
}

void TestEngine::testProfile_InvalidSettings() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto profileDir = QDir(dir.path()).filePath(QStringLiteral("default"));
	QVERIFY(QDir().mkpath(profileDir));
	const auto settings = QDir(profileDir).filePath(QStringLiteral("appearance.yaml"));

	Profile profile(QStringLiteral("default"), profileDir);

	// Настройки должны быть отображением (map), тема — строкой.
	QVERIFY(writeFile(settings, "- theme\n"));
	QVERIFY(!profile.load(defaultAppearance()));

	QVERIFY(writeFile(settings, "theme: [dark]\n"));
	QVERIFY(!profile.load(defaultAppearance()));

	QVERIFY(writeFile(settings, "application: base\n"));
	QVERIFY(!profile.load(defaultAppearance()));

	// Некорректный YAML не должен подменяться значениями по умолчанию.
	QVERIFY(writeFile(settings, "theme: [\n"));
	QVERIFY(!profile.load(defaultAppearance()));
}

void TestEngine::testProfile_UserProfileAllowsEncryptedStores() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	Profile profile(QStringLiteral("abc123"), QDir(dir.path()).filePath(QStringLiteral("abc123")));
	QVERIFY(profile.load(defaultAppearance()));

	QVERIFY(!profile.isDefault());
	QVERIFY(profile.allowsEncryptedStores());
}

void TestEngine::testProfile_SelectsProfileDirectory() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	Resources resources;
	resources.Directories.set(QStringLiteral("path.profiles"), QDir(dir.path()));

	QVERIFY(resources.selectProfile(QStringLiteral("default")));
	QCOMPARE(resources.currentProfileName(), QStringLiteral("default"));
	QVERIFY(resources.currentProfilePath());
	QVERIFY(QDir(dir.path()).exists(QStringLiteral("default")));

	// Имя нормализуется к нижнему регистру и не должно выходить из каталога.
	QVERIFY(resources.selectProfile(QStringLiteral("ABC123")));
	QCOMPARE(resources.currentProfileName(), QStringLiteral("abc123"));
	QVERIFY(QDir(dir.path()).exists(QStringLiteral("abc123")));

	QVERIFY(!resources.selectProfile(QStringLiteral("../escape")));
	QCOMPARE(resources.currentProfileName(), QStringLiteral("abc123"));
	QVERIFY(!resources.selectProfile(QString()));
}

void TestEngine::testProfile_HidesEncryptedStoresWithoutAccess() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	Resources resources;
	resources.Directories.set(QStringLiteral("path.profiles"), QDir(dir.path()));

	// Пока профиль не выбран, ограничение не действует: сервисы могут
	// работать без загруженного профиля.
	QVERIFY(resources.allowsEncryptedStores());

	// Профиль неавторизованного пользователя — только открытые базы.
	QVERIFY(resources.selectProfile(QStringLiteral("default")));
	QVERIFY(!resources.allowsEncryptedStores());

	// Профиль авторизованного пользователя получает зашифрованные базы.
	QVERIFY(resources.selectProfile(QStringLiteral("abc123")));
	QVERIFY(resources.allowsEncryptedStores());

	// Выход из системы возвращает ограничение профиля default.
	QVERIFY(resources.selectProfile(QStringLiteral("default")));
	QVERIFY(!resources.allowsEncryptedStores());
}

void TestEngine::testProfile_BootstrapManagerAppliesProfile() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	Resources resources;
	// bootstrap.yaml и тема берутся из эталонной конфигурации, а настройки
	// профиля пишутся во временный каталог.
	resources.Directories.set(QStringLiteral("path.profile"),
		QDir(QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR)));
	resources.Directories.set(QStringLiteral("path.profiles"), QDir(dir.path()));
	QVERIFY(resources.selectProfile(QStringLiteral("default")));

	BootstrapManager manager(&resources);
	QVERIFY(manager.reload());
	QCOMPARE(manager.profileName(), QStringLiteral("default"));
	QCOMPARE(manager.config().appearance.themeName, QStringLiteral("dark"));
	QCOMPARE(manager.config().appearance.application.name, QStringLiteral("base"));
	QCOMPARE(manager.config().appearance.application.viewport, QStringLiteral("BaseViewport"));

	// Первый запуск создал настройки профиля из appearance в bootstrap.yaml.
	const auto settingsPath = QDir(dir.path()).filePath(QStringLiteral("default/appearance.yaml"));
	QVERIFY(QFile::exists(settingsPath));

	// Правка профиля перекрывает значения bootstrap при следующем запуске.
	QVERIFY(writeFile(settingsPath, R"(theme: dark
application:
  name: singletone
  viewport:
    xtype: SingletoneViewport
)"));
	QVERIFY(manager.reload());
	QCOMPARE(manager.config().appearance.themeName, QStringLiteral("dark"));
	QCOMPARE(manager.config().appearance.application.name, QStringLiteral("singletone"));
	QCOMPARE(manager.config().appearance.application.viewport, QStringLiteral("SingletoneViewport"));
}
