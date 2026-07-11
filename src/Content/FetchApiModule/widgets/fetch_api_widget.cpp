#include "Content/FetchApiModule/widgets/fetch_api_widget.h"
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

	void setupUI();
};

FetchApiWidget::FetchApiWidget(QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {
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
	QComboBox* methodCombo = new QComboBox();
	methodCombo->addItems({ "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD" });
	methodCombo->setMinimumWidth(110);
	methodCombo->setStyleSheet("QComboBox { padding: 5px; font-weight: bold; }");

	// Поле ввода URL
	QLineEdit* urlEdit = new QLineEdit();
	urlEdit->setPlaceholderText("Введите URL запроса (например, https://api.example.com/v1/users)");
	urlEdit->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #ccc; border-radius: 4px; }");

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
		
		QPushButton:hover {
		  background-color: #4a5568;
		  border: 1px solid #718096;
		
		QPushButton:pressed {
		  background-color: #1a202c;
		})"
	);

	topBarLayout->addWidget(methodCombo);
	topBarLayout->addWidget(urlEdit, 1);
	topBarLayout->addWidget(sendBtn);

	// Разделитель (Request и Response)
	QSplitter* splitter = new QSplitter(Qt::Vertical);

	// Область запроса (request)
	QWidget* requestWidget = new QWidget();
	QVBoxLayout* reqLayout = new QVBoxLayout(requestWidget);
	reqLayout->setContentsMargins(0, 0, 0, 0);

	QTabWidget* reqTabs = new QTabWidget();

	// Вкладка Body
	QPlainTextEdit* bodyEdit = new QPlainTextEdit();
	bodyEdit->setPlaceholderText("{\n  \"key\": \"value\"\n}");
	QFont monoFont("Consolas", 11); // Моноширинный шрифт для кода
	if (!monoFont.exactMatch()) monoFont = QFont("Courier New", 11);
	bodyEdit->setFont(monoFont);
	bodyEdit->setStyleSheet("QPlainTextEdit { background-color: #2D2D2D; color: #F8F8F2; border: 1px solid #444; }");

	reqTabs->addTab(bodyEdit, "Body");
	reqTabs->addTab(new QWidget(), "Headers");
	reqTabs->addTab(new QWidget(), "Params");
	reqTabs->addTab(new QWidget(), "Auth");

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

	QPlainTextEdit* respBodyEdit = new QPlainTextEdit();
	respBodyEdit->setReadOnly(true);
	respBodyEdit->setFont(monoFont);
	respBodyEdit->setPlaceholderText("Ответ сервера появится здесь...");
	respBodyEdit->setStyleSheet("QPlainTextEdit { background-color: #2D2D2D; color: #F8F8F2; border: 1px solid #444; }"); // Темная тема для ответа

	respTabs->addTab(respBodyEdit, "Body");
	respTabs->addTab(new QWidget(), "Cookies");
	respTabs->addTab(new QWidget(), "Headers");
	respTabs->addTab(new QWidget(), "Test Results");

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
