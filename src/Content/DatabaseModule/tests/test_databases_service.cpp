#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include "Libs/Resources/resources.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class TestController : public CommandController {
public:
  explicit TestController(Resources* resources) : CommandController(resources) {}
  CommandProcessor* commandProcessor() const override { return nullptr; }
  ServicesRegistry* services() const override { return nullptr; }
private:
  void initContext() override {}
};

class TestDatabasesService : public QObject {
  Q_OBJECT
private slots:
  void defaultAliasAndCaseSensitivePath();
  void relativeConfiguredPath();
  void chronologicalMultiStatementMigrations();
  void retryAfterFailedMigration();
  void encryptedDatabase();
  void encryptedDatabaseRestrictedByProfile();
  void rollbackIsAtomic();
};

static QVariantMap migration(qint64 version, const QString& up, const QString& down = {}) {
  return {{"version", version}, {"description", QString::number(version)}, {"up", up}, {"down", down}};
}

void TestDatabasesService::defaultAliasAndCaseSensitivePath() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  Resources resources;
  resources.Directories.set("path.databases", QDir(dir.path()).filePath("MixedCase"));
  TestController controller(&resources);
  DatabasesService service(&resources, controller.globalContext());
  auto* db = service.connection("Custom");
  QVERIFY(db);
  QCOMPARE(db->databaseName(), QDir(dir.path()).filePath("MixedCase/custom.db"));
  QCOMPARE(service.connection("CUSTOM"), db);
  QVERIFY(!service.connection("../escape"));
  service.cleadEncryptionKey();
}

void TestDatabasesService::relativeConfiguredPath() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  Resources resources;
  resources.Directories.set("path.profile", dir.path());
  TestController controller(&resources);
  auto store = controller.globalContext()->createScopedContext("store");
  auto ctx = store->createScopedContext("custom");
  ctx->setVariable("path", "CustomDir/MyData.db");
  DatabasesService service(&resources, controller.globalContext());
  auto* db = service.connection("custom");
  QVERIFY(db);
  QCOMPARE(db->databaseName(), QDir(dir.path()).filePath("CustomDir/MyData.db"));
}

void TestDatabasesService::chronologicalMultiStatementMigrations() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  Resources resources;
  resources.Directories.set("path.databases", dir.path());
  TestController controller(&resources);
  auto store = controller.globalContext()->createScopedContext("store");
  auto ctx = store->createScopedContext("sample");
  const qint64 first = 20251113181900LL;
  const qint64 second = 20261113181900LL;
  ctx->setVariable("migrations", QVariantList{
    migration(second, "INSERT INTO timeline VALUES (2);"),
    migration(first, "CREATE TABLE timeline (value INTEGER); INSERT INTO timeline VALUES (1);")
  });
  DatabasesService service(&resources, controller.globalContext());
  auto* db = service.connection("sample");
  QVERIFY(db);
  auto reader = db->executeQuery("SELECT value FROM timeline ORDER BY rowid");
  QVERIFY(reader);
  QVERIFY(reader->next());
  QCOMPARE(reader->value(0).toInt(), 1);
  QVERIFY(reader->next());
  QCOMPARE(reader->value(0).toInt(), 2);
  reader.reset();
  auto versions = db->executeQuery("SELECT version FROM _migrations ORDER BY version");
  QVERIFY(versions);
  QVERIFY(versions->next());
  QCOMPARE(versions->value(0).toLongLong(), first);
  QVERIFY(versions->next());
  QCOMPARE(versions->value(0).toLongLong(), second);
  versions.reset();
  service.reloadAliases();
  ctx->setVariable("migrations", QVariantList{});
  QVERIFY(service.connection("sample"));
}

void TestDatabasesService::retryAfterFailedMigration() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  Resources resources;
  resources.Directories.set("path.databases", dir.path());
  TestController controller(&resources);
  auto store = controller.globalContext()->createScopedContext("store");
  auto ctx = store->createScopedContext("retry");
  ctx->setVariable("migrations", QVariantList{migration(1, "CREATE TABLE retry (v INTEGER); INVALID SQL;")});
  DatabasesService service(&resources, controller.globalContext());
  QVERIFY(!service.connection("retry"));
  ctx->setVariable("migrations", QVariantList{migration(1, "CREATE TABLE retry (v INTEGER); INSERT INTO retry VALUES (42);")});
  auto* db = service.connection("retry");
  QVERIFY(db);
  auto reader = db->executeQuery("SELECT v FROM retry");
  QVERIFY(reader);
  QVERIFY(reader->next());
  QCOMPARE(reader->value(0).toInt(), 42);
}

void TestDatabasesService::encryptedDatabase() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  Resources resources;
  resources.Directories.set("path.databases", dir.path());
  TestController controller(&resources);
  auto store = controller.globalContext()->createScopedContext("store");
  auto ctx = store->createScopedContext("secret");
  ctx->setVariable("encrypted", true);
  DatabasesService service(&resources, controller.globalContext());
  QVERIFY(!service.connection("secret"));
  service.setEncryptionKey(QByteArray(32, 'a'));
  auto* db = service.connection("secret");
  QVERIFY(db);
  QVERIFY(db->execute("CREATE TABLE secret (v TEXT); INSERT INTO secret VALUES ('private');"));
  service.cleadEncryptionKey();
  QFile file(QDir(dir.path()).filePath("secret.db"));
  QVERIFY(file.open(QIODevice::ReadOnly));
  QVERIFY(file.read(16) != QByteArray("SQLite format 3\0", 16));
  file.close();
  service.setEncryptionKey(QByteArray(32, 'b'));
  QVERIFY(!service.connection("secret"));
  service.setEncryptionKey(QByteArray(32, 'a'));
  QVERIFY(service.connection("secret"));
}

void TestDatabasesService::encryptedDatabaseRestrictedByProfile() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  Resources resources;
  resources.Directories.set("path.databases", dir.path());
  resources.Directories.set("path.profiles", QDir(dir.path()).filePath("profiles"));
  TestController controller(&resources);
  auto store = controller.globalContext()->createScopedContext("store");
  auto ctx = store->createScopedContext("secret");
  ctx->setVariable("encrypted", true);
  DatabasesService service(&resources, controller.globalContext());
  service.setEncryptionKey(QByteArray(32, 'a'));

  // Профиль неавторизованного пользователя: зашифрованная база недоступна,
  // даже если ключ установлен.
  QVERIFY(resources.selectProfile("default"));
  QVERIFY(!resources.allowsEncryptedStores());
  QVERIFY(!service.connection("secret"));

  // Профиль авторизованного пользователя открывает базу.
  QVERIFY(resources.selectProfile("abc123"));
  QVERIFY(resources.allowsEncryptedStores());
  QVERIFY(service.connection("secret"));

  // Возврат к default закрывает доступ, в том числе к уже открытому
  // соединению.
  QVERIFY(resources.selectProfile("default"));
  QVERIFY(!service.connection("secret"));
}

void TestDatabasesService::rollbackIsAtomic() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  SQLiteConnection db;
  QVERIFY(db.open("file=" + QDir(dir.path()).filePath("rollback.db") + ";"));
  MigrationManager manager;
  manager.addMigration(20251113181900LL, "init",
    [](SQLiteConnection& c) { return c.execute("CREATE TABLE stuff (v INTEGER);"); },
    [](SQLiteConnection& c) { return c.execute("DROP TABLE stuff;"); });
  QVERIFY(manager.migrate(db));
  QVERIFY(db.execute("CREATE TRIGGER prevent_delete BEFORE DELETE ON _migrations BEGIN SELECT RAISE(ABORT, 'blocked'); END;"));
  QVERIFY(!manager.rollback(db));
  auto reader = db.executeQuery("SELECT count(*) FROM stuff");
  QVERIFY(reader);
  QVERIFY(reader->next());
  QCOMPARE(reader->value(0).toInt(), 0);
  reader.reset();
  QCOMPARE(manager.currentVersion(db), 20251113181900LL);
}

QTEST_GUILESS_MAIN(TestDatabasesService)
#include "test_databases_service.moc"
