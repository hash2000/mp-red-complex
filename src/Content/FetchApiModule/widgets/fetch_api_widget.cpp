#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
#include "Content/FetchApiModule/services/fetch_service.h"
#include "Content/BaseWidgets/key_value/key_value_editor_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QFont>
#include <QGroupBox>

class FetchApiWidget::Private {
public:
	Private(FetchApiWidget* parent) : q(parent) {}
	FetchApiWidget* q;

	FetchApiService* fetchApiService = nullptr;
	QLineEdit* requestUrlEdit = nullptr;
	QComboBox* requestMethodCombo = nullptr;
	QPlainTextEdit* requestBodyEdit = nullptr;
	KeyValueEditorWidget* requestHeaders = nullptr;
	KeyValueEditorWidget* requestParams = nullptr;
	QWidget* requestAuth = nullptr;

	QPlainTextEdit* responseBodyEdit = nullptr;
	KeyValueEditorWidget* responseCoocies = nullptr;
	KeyValueEditorWidget* responseHeaders = nullptr;

	void setupUI();
};

FetchApiWidget::FetchApiWidget(FetchApiService* fetchApiService, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {
	d->fetchApiService = fetchApiService;
	d->setupUI();
}

FetchApiWidget::~FetchApiWidget() = default;

void FetchApiWidget::Private::setupUI() {
	// Главный вертикальный layout
	QVBoxLayout* mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(15, 15, 15, 15);
	mainLayout->setSpacing(10);

	// Верхняя панель (Метод, URL, Кнопка Send)
	QHBoxLayout* topBarLayout = new QHBoxLayout();

	// Выпадающий список методов
	requestMethodCombo = new QComboBox();
	requestMethodCombo->addItems({ "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD" });
	requestMethodCombo->setMinimumWidth(110);
	requestMethodCombo->setStyleSheet("QComboBox { padding: 5px; font-weight: bold; }");

	// Поле ввода URL
	requestUrlEdit = new QLineEdit();
	requestUrlEdit->setPlaceholderText("Введите URL запроса (например, https://api.example.com/v1/users)");
	requestUrlEdit->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #ccc; border-radius: 4px; }");

	// Кнопка Send 
	QPushButton* sendBtn = new QPushButton("Send");
	sendBtn->setMinimumWidth(90);
	sendBtn->setCursor(Qt::PointingHandCursor);
	sendBtn->setStyleSheet(R"(
		QPushButton {
		  background-color: #2d3748;
		  color: #e2e8f0;
		  border: 1px solid #4a5568;
		  border-radius: 3px;
		  font-size: 12px;
		}
		QPushButton:hover {
		  background-color: #4a5568;
		  border: 1px solid #718096;
		}
		QPushButton:pressed {
		  background-color: #1a202c;
		})"
	);

	topBarLayout->addWidget(requestMethodCombo);
	topBarLayout->addWidget(requestUrlEdit, 1);
	topBarLayout->addWidget(sendBtn);

	// Разделитель (Request и Response)
	QSplitter* splitter = new QSplitter(Qt::Vertical);

	// Область запроса (request)
	QWidget* requestWidget = new QWidget();
	QVBoxLayout* reqLayout = new QVBoxLayout(requestWidget);
	reqLayout->setContentsMargins(0, 0, 0, 0);

	QTabWidget* reqTabs = new QTabWidget();

	// Вкладка Body
	requestBodyEdit = new QPlainTextEdit();
	requestBodyEdit->setPlaceholderText("{\n  \"key\": \"value\"\n}");
	QFont monoFont("Consolas", 11); // Моноширинный шрифт для кода
	if (!monoFont.exactMatch()) {
		monoFont = QFont("Courier New", 11);
	}
	requestBodyEdit->setFont(monoFont);
	requestBodyEdit->setStyleSheet("QPlainTextEdit { background-color: #2D2D2D; color: #F8F8F2; border: 1px solid #444; }");

	requestHeaders = new KeyValueEditorWidget();
	requestParams = new KeyValueEditorWidget();
	requestAuth = new QWidget();

	reqTabs->addTab(requestBodyEdit, "Body");
	reqTabs->addTab(requestHeaders, "Headers");
	reqTabs->addTab(requestParams, "Params");
	reqTabs->addTab(requestAuth, "Auth");

	reqLayout->addWidget(reqTabs);

	// Область ответа (response)
	QWidget* responseWidget = new QWidget();
	QVBoxLayout* respLayout = new QVBoxLayout(responseWidget);
	respLayout->setContentsMargins(0, 0, 0, 0);

	// Строка статуса ответа (как в Postman)
	QHBoxLayout* statusLayout = new QHBoxLayout();
	QLabel* statusLabel = new QLabel("Status: 200 OK");
	statusLabel->setStyleSheet("color: green; font-weight: bold;");

	QLabel* timeLabel = new QLabel("Time: 125 ms");
	QLabel* sizeLabel = new QLabel("Size: 1.2 KB");

	statusLayout->addWidget(statusLabel);
	statusLayout->addStretch();
	statusLayout->addWidget(timeLabel);
	statusLayout->addWidget(sizeLabel);

	// Вкладки ответа
	QTabWidget* respTabs = new QTabWidget();

	responseBodyEdit = new QPlainTextEdit();
	responseBodyEdit->setReadOnly(true);
	responseBodyEdit->setFont(monoFont);
	responseBodyEdit->setPlaceholderText("Ответ сервера появится здесь...");
	responseBodyEdit->setStyleSheet("QPlainTextEdit { background-color: #2D2D2D; color: #F8F8F2; border: 1px solid #444; }"); // Темная тема для ответа

	responseCoocies = new KeyValueEditorWidget();
	responseHeaders = new KeyValueEditorWidget();

	respTabs->addTab(responseBodyEdit, "Body");
	respTabs->addTab(responseCoocies, "Cookies");
	respTabs->addTab(responseHeaders, "Headers");

	respLayout->addLayout(statusLayout);
	respLayout->addWidget(respTabs);

	// Добавляем виджеты в разделитель
	splitter->addWidget(requestWidget);
	splitter->addWidget(responseWidget);

	// Задаем начальные пропорции (50% на запрос, 50% на ответ)
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 1);

	mainLayout->addLayout(topBarLayout);
	mainLayout->addWidget(splitter, 1);
}

void FetchApiWidget::onSendClicked() {
	//d->requestHeaders
}
