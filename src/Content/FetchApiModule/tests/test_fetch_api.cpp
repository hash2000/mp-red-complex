#include "Content/FetchApiModule/documents/fetch_api_document.h"
#include "Content/FetchApiModule/models/fetch_api_result.h"
#include "Content/FetchApiModule/models/fetch_opt.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"
#include "Libs/BaseWidgets/key_value/key_value_editor_widget.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QComboBox>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QToolButton>
#include <QUrlQuery>
#include <QtTest/QtTest>

#include <memory>

namespace {

/// Значение из карты пар «имя → значение» (пустая строка, если нет).
QString valueOf(const FetchApiKeyValues& values, const QString& name) {
	const auto it = values.find(name);
	return it == values.end() ? QString() : it->second;
}

std::shared_ptr<CommandContext> makeGlobalContext() {
	return std::make_shared<CommandContext>(nullptr, nullptr, QString());
}

/// Редактор свойства по идентификатору в PropertiesListWidget.
QWidget* findPropertyEditor(QWidget* root, const QString& id) {
	for (auto* child : root->findChildren<QWidget*>()) {
		if (child->property("propertyId").toString() == id) {
			return child;
		}
	}

	return nullptr;
}

/// Кнопка ленты с указанной подписью.
RibbonButton* findRibbonButton(QWidget* root, const QString& label) {
	for (auto* button : root->findChildren<RibbonButton*>()) {
		if (button->label() == label) {
			return button;
		}
	}

	return nullptr;
}

} // namespace

class TestFetchApi : public QObject {
	Q_OBJECT

private slots:
	void keyValuesParsing();
	void methodConversions();
	void sslPolicyConversions();
	void handlerConversions();

	void requestBuilding();
	void requestKeepsExplicitContentType();
	void requestKeepsEmptyParameter();

	void resultInContext();
	void resultErrorInContext();
	void resultCookies();

	void serviceValidation();
	void serviceExecutesRequest();

	void widgetParameterRoundTrip();
	void widgetAuthentication();
	void widgetShowsResult();

	void documentExposesSendProperties();
	void documentRequestPropertiesChangeWidget();
	void documentRequestPropertiesWriteContext();
	void documentRibbonSendsRequest();
};

void TestFetchApi::keyValuesParsing() {
	const auto semicolon = FetchApiFormat::parseKeyValues(QStringLiteral("a=1; b=2; c="));
	QCOMPARE(static_cast<int>(semicolon.size()), 3);
	QCOMPARE(valueOf(semicolon, QStringLiteral("a")), QStringLiteral("1"));
	QCOMPARE(valueOf(semicolon, QStringLiteral("b")), QStringLiteral("2"));
	QCOMPARE(valueOf(semicolon, QStringLiteral("c")), QString());

	const auto lines = FetchApiFormat::parseKeyValues(
		QStringLiteral("Content-Type: application/json\nX-Url: https://host/path"));
	QCOMPARE(static_cast<int>(lines.size()), 2);
	QCOMPARE(valueOf(lines, QStringLiteral("Content-Type")), QStringLiteral("application/json"));
	QCOMPARE(valueOf(lines, QStringLiteral("X-Url")), QStringLiteral("https://host/path"));

	const auto query = FetchApiFormat::parseKeyValues(QStringLiteral("?page=2&size=10"));
	QCOMPARE(static_cast<int>(query.size()), 2);
	QCOMPARE(valueOf(query, QStringLiteral("page")), QStringLiteral("2"));

	const auto commented = FetchApiFormat::parseKeyValues(
		QStringLiteral("# заголовок\nAccept: text/plain"));
	QCOMPARE(static_cast<int>(commented.size()), 1);
	QCOMPARE(valueOf(commented, QStringLiteral("Accept")), QStringLiteral("text/plain"));

	// Обратное форматирование читается тем же разбором.
	const auto restored = FetchApiFormat::parseKeyValues(FetchApiFormat::formatKeyValues(lines));
	QCOMPARE(static_cast<int>(restored.size()), 2);
	QCOMPARE(valueOf(restored, QStringLiteral("X-Url")), QStringLiteral("https://host/path"));
}

void TestFetchApi::methodConversions() {
	QCOMPARE(Format<FetchApiMethod>::format(FetchApiMethod::Get), QStringLiteral("GET"));
	QCOMPARE(Format<FetchApiMethod>::format(FetchApiMethod::Patch), QStringLiteral("PATCH"));

	QVERIFY(From<FetchApiMethod>::from(QStringLiteral("post")).has_value());
	QCOMPARE(*From<FetchApiMethod>::from(QStringLiteral("PoSt")), FetchApiMethod::Post);
	QCOMPARE(*From<FetchApiMethod>::from(QStringLiteral(" head ")), FetchApiMethod::Head);
	QVERIFY(!From<FetchApiMethod>::from(QStringLiteral("options")).has_value());
}

void TestFetchApi::sslPolicyConversions() {
	QCOMPARE(Format<FetchApiSslPolicy>::format(FetchApiSslPolicy::TrustCustomCa),
		QStringLiteral("trust-custom-ca"));
	QVERIFY(From<FetchApiSslPolicy>::from(QStringLiteral("ignore-errors")).has_value());
	QCOMPARE(*From<FetchApiSslPolicy>::from(QStringLiteral("TRUST_CUSTOM_CA")),
		FetchApiSslPolicy::TrustCustomCa);
	QCOMPARE(*From<FetchApiSslPolicy>::from(QString()), FetchApiSslPolicy::Strict);
	QVERIFY(!From<FetchApiSslPolicy>::from(QStringLiteral("weak")).has_value());
}

void TestFetchApi::handlerConversions() {
	QCOMPARE(Format<FetchApiHandlerType>::format(FetchApiHandlerType::Rest),
		QStringLiteral("rest"));
	QCOMPARE(*From<FetchApiHandlerType>::from(QString()), FetchApiHandlerType::Rest);
	QCOMPARE(*From<FetchApiHandlerType>::from(QStringLiteral("REST")),
		FetchApiHandlerType::Rest);
	QVERIFY(!From<FetchApiHandlerType>::from(QStringLiteral("grpc")).has_value());
}

void TestFetchApi::requestBuilding() {
	FetchApiOpt options(QStringLiteral("example.com/api"),
		{ { QStringLiteral("q"), QStringLiteral("hello world") } },
		{ { QStringLiteral("X-Test"), QStringLiteral("1") } });

	QVERIFY(options.setMethod(QStringLiteral("post")));
	QVERIFY(options.setHandler(QStringLiteral("rest")));
	options.setBodyText(QStringLiteral(R"({"b":2,"a":1})"));

	QVERIFY(options.isValid());
	QCOMPARE(QString::fromLatin1(options.httpMethodName()), QStringLiteral("POST"));
	QCOMPARE(options.describe(),
		QStringLiteral("POST example.com/api"));

	const auto request = options.request();
	QCOMPARE(request.url().scheme(), QStringLiteral("https"));
	QCOMPARE(request.url().host(), QStringLiteral("example.com"));
	QCOMPARE(request.url().path(), QStringLiteral("/api"));

	const QUrlQuery query(request.url());
	QVERIFY(query.hasQueryItem(QStringLiteral("q")));
	QCOMPARE(query.queryItemValue(QStringLiteral("q"), QUrl::FullyDecoded),
		QStringLiteral("hello world"));

	QCOMPARE(QString::fromUtf8(request.rawHeader("X-Test")), QStringLiteral("1"));

	// Тело без явного Content-Type: JSON определяется по содержимому.
	QCOMPARE(QString::fromUtf8(request.rawHeader("Content-Type")),
		QStringLiteral("application/json"));
}

void TestFetchApi::requestKeepsExplicitContentType() {
	FetchApiOpt options(QStringLiteral("https://example.com"),
		{}, { { QStringLiteral("Content-Type"), QStringLiteral("text/plain") } });
	options.setBodyText(QStringLiteral(R"({"a":1})"));

	const auto request = options.request();
	QCOMPARE(QString::fromUtf8(request.rawHeader("Content-Type")),
		QStringLiteral("text/plain"));
}

void TestFetchApi::requestKeepsEmptyParameter() {
	FetchApiOpt options(QStringLiteral("https://example.com/items"),
		{ { QStringLiteral("flag"), QString() } });

	const QUrlQuery query(options.request().url());
	QVERIFY(query.hasQueryItem(QStringLiteral("flag")));
	QCOMPARE(query.queryItemValue(QStringLiteral("flag")), QString());
}

void TestFetchApi::resultInContext() {
	auto global = makeGlobalContext();
	auto document = std::make_shared<CommandContext>(nullptr, global, QStringLiteral("doc"));
	QSignalSpy events(document.get(), &CommandContext::eventFired);

	FetchApiResult result;
	result.requestId = QStringLiteral("request-1");
	result.method = FetchApiMethod::Post;
	result.location = QStringLiteral("https://api.example.com/items");
	result.url = QUrl(result.location);
	result.statusCode = 201;
	result.statusText = QStringLiteral("Created");
	result.data = QByteArray(R"({"ok":true})");
	result.headers.append(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
	result.failed = false;
	result.elapsedMs = 12;
	result.size = result.data.size();

	result.applyToContext(*document);

	QVERIFY(result.isSuccess());
	QVERIFY(!result.isHttpError());
	QCOMPARE(document->variable(QStringLiteral("data")).toString(),
		QString::fromUtf8(result.data));
	QCOMPARE(document->variable(QStringLiteral("content-type")).toString(),
		QStringLiteral("application/json"));
	QVERIFY(document->variable(QStringLiteral("headers")).toString()
		.contains(QStringLiteral("Content-Type: application/json")));
	QCOMPARE(document->variable(QStringLiteral("status-code")).toInt(), 201);
	QCOMPARE(document->variable(QStringLiteral("status-text")).toString(),
		QStringLiteral("Created"));
	QCOMPARE(document->variable(QStringLiteral("error")).toString(), QString());
	QCOMPARE(document->variable(QStringLiteral("error-code")).toInt(), 0);
	QCOMPARE(document->variable(QStringLiteral("elapsed-ms")).toLongLong(), 12LL);
	QCOMPARE(document->variable(QStringLiteral("size")).toLongLong(),
		static_cast<qlonglong>(result.size));
	QCOMPARE(document->variable(QStringLiteral("method")).toString(),
		QStringLiteral("POST"));
	QCOMPARE(document->variableMimeType(QStringLiteral("data")),
		QStringLiteral("application/json"));

	QCOMPARE(events.count(), 1);
	QCOMPARE(events.takeFirst().at(0).toString(), FetchApiResult::completeEventName());
}

void TestFetchApi::resultErrorInContext() {
	auto global = makeGlobalContext();
	auto document = std::make_shared<CommandContext>(nullptr, global, QStringLiteral("doc"));

	FetchApiResult result;
	result.statusCode = 500;
	result.statusText = QStringLiteral("Internal Server Error");
	result.data = QByteArray("failure");
	result.failed = true;
	result.errorString = QStringLiteral("HTTP 500 Internal Server Error");

	result.applyToContext(*document);

	QVERIFY(!result.isSuccess());
	QVERIFY(result.isHttpError());
	QCOMPARE(document->variable(QStringLiteral("error")).toString(),
		result.errorString);
	QCOMPARE(document->variable(QStringLiteral("data")).toString(),
		QStringLiteral("failure"));
}

void TestFetchApi::resultCookies() {
	FetchApiResult result;
	result.headers.append(QStringLiteral("Set-Cookie"),
		QStringLiteral("sid=abc; Path=/; HttpOnly"));
	result.headers.append(QStringLiteral("Set-Cookie"),
		QStringLiteral("theme=dark; Path=/"));

	const auto cookies = result.cookies();
	QCOMPARE(static_cast<int>(cookies.size()), 2);
	QCOMPARE(cookies.at(0).first, QStringLiteral("sid"));
	QVERIFY(cookies.at(0).second.contains(QStringLiteral("Path=/")));
	QCOMPARE(cookies.at(1).first, QStringLiteral("theme"));
}

void TestFetchApi::serviceValidation() {
	FetchApiService service(nullptr);
	QVERIFY(service.lastError().isEmpty());
	QCOMPARE(service.pendingRequests(), 0);

	bool called = false;
	FetchApiOpt empty;
	QVERIFY(!service.execute(empty, [&called](const FetchApiResult&) { called = true; }));
	QVERIFY(!service.lastError().isEmpty());
	QVERIFY(!called);

	FetchApiOpt invalidHost(QStringLiteral("http://"));
	QVERIFY(!invalidHost.isValid());
	QVERIFY(!service.execute(invalidHost, [&called](const FetchApiResult&) { called = true; }));

	// Политика Trust custom CA без сертификата отклоняется до отправки.
	FetchApiOpt trustCa(QStringLiteral("https://example.com"));
	trustCa.setSslPolicy(FetchApiSslPolicy::TrustCustomCa);
	QVERIFY(!service.execute(trustCa, [&called](const FetchApiResult&) { called = true; }));
	QVERIFY(service.lastError().contains(QStringLiteral("CA")));
	QCOMPARE(service.pendingRequests(), 0);
	QVERIFY(!called);
}

void TestFetchApi::serviceExecutesRequest() {
	// Минимальный HTTP-сервер: принимает один запрос и отвечает JSON-ом.
	QTcpServer server;
	QVERIFY(server.listen(QHostAddress::LocalHost, 0));

	QByteArray received;
	bool responded = false;
	QObject::connect(&server, &QTcpServer::newConnection, &server,
		[&server, &received, &responded]() {
			auto* socket = server.nextPendingConnection();
			QObject::connect(socket, &QTcpSocket::readyRead, socket,
				[socket, &received, &responded]() {
					received += socket->readAll();
					if (responded) {
						return;
					}

					// Отвечаем, когда пришли заголовки и всё тело запроса.
					const int headerEnd = received.indexOf("\r\n\r\n");
					if (headerEnd < 0) {
						return;
					}

					const auto headers = received.left(headerEnd);
					const auto lowerHeaders = headers.toLower();
					int contentLength = 0;
					const int lengthPos = lowerHeaders.indexOf("content-length:");
					if (lengthPos >= 0) {
						const int lineEnd = lowerHeaders.indexOf("\r\n", lengthPos);
						const auto value = headers.mid(lengthPos + 15,
							lineEnd < 0 ? -1 : lineEnd - lengthPos - 15).trimmed();
						contentLength = value.toInt();
					}

					if (received.size() - headerEnd - 4 < contentLength) {
						return;
					}

					responded = true;

					const QByteArray body = R"({"ok":true})";
					const QByteArray response =
						"HTTP/1.1 200 OK\r\n"
						"Content-Type: application/json\r\n"
						"Set-Cookie: sid=abc; Path=/\r\n"
						"Content-Length: " + QByteArray::number(body.size()) + "\r\n"
						"Connection: close\r\n\r\n" + body;

					socket->write(response);
					socket->flush();
					socket->disconnectFromHost();
				});
		});

	FetchApiService service(nullptr);
	FetchApiOpt options(QStringLiteral("http://127.0.0.1:%1/test")
		.arg(server.serverPort()));
	QVERIFY(options.setMethod(QStringLiteral("post")));
	options.setHeadersFormatted(QStringLiteral("X-Test: 1"));
	options.setBodyText(QStringLiteral(R"({"a":1})"));

	FetchApiResult result;
	bool finished = false;
	const bool started = service.execute(options,
		[&result, &finished](const FetchApiResult& value) {
			result = value;
			finished = true;
		});
	QVERIFY(started);
	QTRY_VERIFY_WITH_TIMEOUT(finished, 5000);

	QVERIFY2(!result.failed, qPrintable(result.errorMessage()));
	QCOMPARE(result.statusCode, 200);
	QCOMPARE(result.statusText, QStringLiteral("OK"));
	QCOMPARE(result.text(), QStringLiteral(R"({"ok":true})"));
	QCOMPARE(result.contentType(), QStringLiteral("application/json"));
	QCOMPARE(static_cast<int>(result.cookies().size()), 1);
	QCOMPARE(result.cookies().first().first, QStringLiteral("sid"));
	QCOMPARE(result.size, static_cast<qint64>(11));

	QVERIFY(received.startsWith("POST /test HTTP/1.1"));
	QVERIFY(received.contains("X-Test: 1"));
	QVERIFY(received.contains("Content-Type: application/json"));
	QVERIFY(received.endsWith(R"({"a":1})"));
}

void TestFetchApi::widgetParameterRoundTrip() {
	FetchApiWidget widget(nullptr, nullptr, nullptr);

	widget.setLocation(QStringLiteral("https://api.example.com/v1/items"));
	widget.setMethod(FetchApiMethod::Put);
	widget.setBodyText(QStringLiteral(R"({"name":"x"})"));
	widget.setParametersFormatted(QStringLiteral("page=2; size=10"));
	widget.setHeadersFormatted(QStringLiteral("X-Test: 1; Accept: application/json"));
	widget.setSslPolicy(FetchApiSslPolicy::TrustCustomCa);
	widget.setCaFile(QStringLiteral("C:/certs/ca.pem"));
	widget.setHandler(FetchApiHandlerType::Rest);

	QCOMPARE(widget.parametersFormatted(), QStringLiteral("page: 2\nsize: 10"));
	QCOMPARE(widget.headersFormatted(),
		QStringLiteral("Accept: application/json\nX-Test: 1"));

	const auto options = widget.requestOptions();
	QCOMPARE(options.location(), widget.location());
	QCOMPARE(options.method(), FetchApiMethod::Put);
	QCOMPARE(QString::fromUtf8(options.body()), QStringLiteral(R"({"name":"x"})"));
	QCOMPARE(valueOf(options.parameters(), QStringLiteral("page")), QStringLiteral("2"));
	QCOMPARE(valueOf(options.parameters(), QStringLiteral("size")), QStringLiteral("10"));
	QCOMPARE(valueOf(options.headers(), QStringLiteral("X-Test")), QStringLiteral("1"));
	QCOMPARE(options.sslPolicy(), FetchApiSslPolicy::TrustCustomCa);
	QCOMPARE(options.handler(), FetchApiHandlerType::Rest);
}

void TestFetchApi::widgetAuthentication() {
	FetchApiWidget widget(nullptr, nullptr, nullptr);
	widget.setLocation(QStringLiteral("https://example.com"));

	auto* authType = widget.findChild<QComboBox*>(QStringLiteral("FetchAuthTypeCombo"));
	auto* nameEdit = widget.findChild<QLineEdit*>(QStringLiteral("FetchAuthNameEdit"));
	auto* valueEdit = widget.findChild<QLineEdit*>(QStringLiteral("FetchAuthValueEdit"));
	QVERIFY(authType);
	QVERIFY(nameEdit);
	QVERIFY(valueEdit);

	// Basic: Authorization: Basic base64(логин:пароль)
	authType->setCurrentIndex(1);
	nameEdit->setText(QStringLiteral("user"));
	valueEdit->setText(QStringLiteral("secret"));

	auto options = widget.requestOptions();
	QCOMPARE(valueOf(options.headers(), QStringLiteral("Authorization")),
		QStringLiteral("Basic dXNlcjpzZWNyZXQ="));

	// Bearer: токен в поле имени.
	authType->setCurrentIndex(2);
	nameEdit->setText(QStringLiteral("token-123"));
	options = widget.requestOptions();
	QCOMPARE(valueOf(options.headers(), QStringLiteral("Authorization")),
		QStringLiteral("Bearer token-123"));

	// API key в query-параметр.
	authType->setCurrentIndex(3);
	nameEdit->setText(QStringLiteral("api_key"));
	valueEdit->setText(QStringLiteral("42"));
	auto* addTo = widget.findChild<QComboBox*>(QStringLiteral("FetchAuthAddToCombo"));
	QVERIFY(addTo);
	addTo->setCurrentIndex(1);
	options = widget.requestOptions();
	QCOMPARE(valueOf(options.parameters(), QStringLiteral("api_key")), QStringLiteral("42"));
	QVERIFY(options.headers().find(QStringLiteral("api_key")) == options.headers().end());
}

void TestFetchApi::widgetShowsResult() {
	FetchApiWidget widget(nullptr, nullptr, nullptr);

	FetchApiResult result;
	result.statusCode = 200;
	result.statusText = QStringLiteral("OK");
	result.data = QByteArray(R"({"a":1})");
	result.headers.append(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
	result.headers.append(QStringLiteral("Set-Cookie"),
		QStringLiteral("sid=abc; Path=/"));
	result.failed = false;
	result.elapsedMs = 5;
	result.size = result.data.size();

	widget.showResult(result);

	auto* status = widget.findChild<QLabel*>(QStringLiteral("FetchStatus"));
	auto* headers = widget.findChild<KeyValueEditorWidget*>(QStringLiteral("FetchResponseHeaders"));
	auto* cookies = widget.findChild<KeyValueEditorWidget*>(QStringLiteral("FetchResponseCookies"));
	QVERIFY(status);
	QVERIFY(headers);
	QVERIFY(cookies);
	QVERIFY(status->text().contains(QStringLiteral("200")));
	QCOMPARE(static_cast<int>(headers->parameters().size()), 2);
	QCOMPARE(static_cast<int>(cookies->parameters().size()), 1);
	QCOMPARE(cookies->parameters().at(0).name, QStringLiteral("sid"));

	widget.clearResult();
	QVERIFY(status->text().isEmpty());
	QCOMPARE(static_cast<int>(cookies->parameters().size()), 0);
}

void TestFetchApi::documentExposesSendProperties() {
	FetchApiDocument document(nullptr, nullptr, nullptr);
	QWidget host;
	const auto groups = document.createPropertyGroups(&host);

	// «Параметры отправки» — группа свойств кнопки «Отправить».
	QCOMPARE(groups.size(), 1);
	QCOMPARE(groups.first().id, QStringLiteral("request"));
	QCOMPARE(groups.first().title, QStringLiteral("Параметры отправки"));

	auto* properties = qobject_cast<PropertiesListWidget*>(groups.first().editor);
	QVERIFY(properties);
	QCOMPARE(properties->propertyCount(), 3);

	// Тип запроса: сейчас реализован только REST.
	auto* handler = qobject_cast<QComboBox*>(
		findPropertyEditor(properties, QStringLiteral("handler")));
	QVERIFY(handler);
	QCOMPARE(handler->count(), 1);
	QCOMPARE(handler->currentText(), QStringLiteral("REST"));

	// Параметры обработки TLS.
	auto* policy = qobject_cast<QComboBox*>(
		findPropertyEditor(properties, QStringLiteral("ssl-policy")));
	QVERIFY(policy);
	QCOMPARE(policy->count(), 3);
	QCOMPARE(policy->currentText(), QStringLiteral("Strict"));

	QVERIFY(findPropertyEditor(properties, QStringLiteral("ssl-ca-file")));
}

void TestFetchApi::documentRequestPropertiesChangeWidget() {
	FetchApiDocument document(nullptr, nullptr, nullptr);
	QWidget host;
	const auto groups = document.createPropertyGroups(&host);
	auto* properties = qobject_cast<PropertiesListWidget*>(groups.first().editor);
	QVERIFY(properties);
	QVERIFY(document.widget());

	auto* policy = qobject_cast<QComboBox*>(
		findPropertyEditor(properties, QStringLiteral("ssl-policy")));
	QVERIFY(policy);

	// Trust custom CA и Ignore errors меняют настройки виджета.
	policy->setCurrentIndex(1);
	QCOMPARE(document.widget()->sslPolicy(), FetchApiSslPolicy::TrustCustomCa);

	policy->setCurrentIndex(2);
	QCOMPARE(document.widget()->sslPolicy(), FetchApiSslPolicy::IgnoreErrors);
}

void TestFetchApi::documentRequestPropertiesWriteContext() {
	auto global = makeGlobalContext();
	auto context = std::make_shared<CommandContext>(nullptr, global, QStringLiteral("doc"));

	FetchApiDocument document(nullptr, nullptr, nullptr);
	document.setContext(context);

	QWidget host;
	const auto groups = document.createPropertyGroups(&host);
	auto* properties = qobject_cast<PropertiesListWidget*>(groups.first().editor);
	QVERIFY(properties);

	auto* policy = qobject_cast<QComboBox*>(
		findPropertyEditor(properties, QStringLiteral("ssl-policy")));
	QVERIFY(policy);
	policy->setCurrentIndex(1);

	// Настройка сохраняется в контекст документа, как и параметры запроса.
	QCOMPARE(context->ownVariables().value(QStringLiteral("ssl-policy")).toString(),
		QStringLiteral("trust-custom-ca"));
	QCOMPARE(document.variable(QStringLiteral("ssl-policy")).toString(),
		QStringLiteral("trust-custom-ca"));
}

void TestFetchApi::documentRibbonSendsRequest() {
	FetchApiDocument document(nullptr, nullptr, nullptr);
	QVERIFY(document.widget());

	auto* send = findRibbonButton(&document, QStringLiteral("Отправить"));
	QVERIFY(send);
	QCOMPARE(send->propertiesGroup(), QStringLiteral("request"));
	QVERIFY(send->propertiesButton());

	// Без HTTP-сервиса отправка сообщает об ошибке, но кнопка ленты — её
	// единственная точка запуска.
	QSignalSpy failed(document.widget(), &FetchApiWidget::requestFailed);
	send->mainButton()->click();
	QCOMPARE(failed.count(), 1);

	QSignalSpy requested(send, &RibbonButton::propertiesRequested);
	send->propertiesButton()->click();
	QCOMPARE(requested.count(), 1);
	QCOMPARE(requested.first().at(0).toString(), QStringLiteral("request"));
}

QTEST_MAIN(TestFetchApi)
#include "test_fetch_api.moc"
