#include "Content/BaseWidgets/key_value/key_value_editor_widget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QMessageBox>
#include <QFont>


class KeyValueEditorWidget::Private {
public:
	Private(KeyValueEditorWidget* parent) : q(parent) {}
	KeyValueEditorWidget* q;

	QStackedWidget* stackedWidget;
	QTableWidget* table;
	QPlainTextEdit* textEdit;
	QPushButton* btnToggleAll;
	QPushButton* btnConvert;
	QPushButton* btnRemoveRow;
	QPushButton* btnAddRow;

	void setupUi();
	void addRow(const QString& param = "", const QString& value = "",
		const QString& desc = "", bool isEnabled = true);
	void removeSelectedRow();
};



KeyValueEditorWidget::KeyValueEditorWidget(QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {
	d->setupUi();
	// Добавляем 3 пустые строки для начала
	for (int i = 0; i < 3; i++) {
		d->addRow();
	}
}

KeyValueEditorWidget::~KeyValueEditorWidget() = default;

void KeyValueEditorWidget::onToggleAll() {
	// Проверяем, есть ли хоть один выключенный элемент
	bool allChecked = true;
	for (int i = 0; i < d->table->rowCount(); i++) {
		if (d->table->item(i, 3)->checkState() == Qt::Unchecked) {
			allChecked = false;
			break;
		}
	}

	// Если все включены - выключаем все, иначе включаем все
	Qt::CheckState newState = allChecked ? Qt::Unchecked : Qt::Checked;
	for (int i = 0; i < d->table->rowCount(); i++) {
		d->table->item(i, 3)->setCheckState(newState);
	}
}

void KeyValueEditorWidget::onConvert() {
	if (d->stackedWidget->currentIndex() == 0) {
		// Режим: Таблица -> Текст
		QString text;
		for (int i = 0; i < d->table->rowCount(); i++) {
			QString param = d->table->item(i, 0)->text();
			QString value = d->table->item(i, 1)->text();
			QString desc = d->table->item(i, 2)->text();
			bool isEnabled = d->table->item(i, 3)->checkState() == Qt::Checked;

			text += QString("<parameter>%1</parameter><value>%2</value>"
				"<description>%3</description><option>%4</option>\n")
				.arg(param, value, desc, isEnabled ? "вкл" : "выкл");
		}
		d->textEdit->setPlainText(text);
		d->stackedWidget->setCurrentIndex(1); // Переключаем на текст
		d->btnConvert->setText("Преобразовать в таблицу");
	}
	else {
		// Режим: Текст -> Таблица
		QString text = d->textEdit->toPlainText();

		// Регулярное выражение для парсинга нашего формата
		QRegularExpression re("<parameter>(.*?)</parameter>\\s*"
			"<value>(.*?)</value>\\s*"
			"<description>(.*?)</description>\\s*"
			"<option>(.*?)</option>");

		QRegularExpressionMatchIterator it = re.globalMatch(text);

		d->table->setRowCount(0); // Очищаем таблицу

		while (it.hasNext()) {
			QRegularExpressionMatch match = it.next();
			QString param = match.captured(1);
			QString value = match.captured(2);
			QString desc = match.captured(3);
			bool isEnabled = (match.captured(4).trimmed() == "вкл");

			d->addRow(param, value, desc, isEnabled);
		}

		// Добавляем одну пустую строку в конец для удобства
		d->addRow();

		d->stackedWidget->setCurrentIndex(0); // Переключаем на таблицу
		d->btnConvert->setText("Преобразовать в текст");
	}
}

void KeyValueEditorWidget::onAddRow() {
	d->addRow();
}

void KeyValueEditorWidget::onRemoveSelectedRow() {
	d->removeSelectedRow();
}

void KeyValueEditorWidget::Private::setupUi() {
	QHBoxLayout* mainLayout = new QHBoxLayout(q);
	mainLayout->setContentsMargins(10, 10, 10, 10);

	// --- ЛЕВАЯ ЧАСТЬ: Таблица и Текст ---
	stackedWidget = new QStackedWidget();

	// 1. Таблица
	table = new QTableWidget(0, 4);
	table->setHorizontalHeaderLabels({ "Имя", "Значение", "Описание", "Вкл/Выкл" });
	table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
	table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	table->verticalHeader()->setVisible(false); // Скрываем номера строк
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setStyleSheet(R"(
		QTableWidget {
		  background-color: #2d3748;
		  color: #4a5568;
		  border: 1px solid #4a5568;
		  border-radius: 3px;
		  font-size: 12px;
		}
		QTableWidget::item {
			padding: 2px;
			color: #4a5568;
		}
		QTableWidget::item:selected {
			background-color: #4a5568;
		}
	)");

	// 2. Текстовый редактор
	textEdit = new QPlainTextEdit();
	QFont monoFont("Consolas", 11);
	if (!monoFont.exactMatch()) {
		monoFont = QFont("Courier New", 11);
	}
	textEdit->setFont(monoFont);
	textEdit->setStyleSheet("QPlainTextEdit { background-color: #2d2d2d; color: #f8f8f2; border: 1px solid #ccc; }");

	stackedWidget->addWidget(table);
	stackedWidget->addWidget(textEdit);

	// --- ПРАВАЯ ЧАСТЬ: Кнопки ---
	QVBoxLayout* btnLayout = new QVBoxLayout();
	btnLayout->setSpacing(10);

	btnToggleAll = new QPushButton("Вкл/Выкл все");
	btnConvert = new QPushButton("Преобразовать в текст");
	btnRemoveRow = new QPushButton("- Удалить строку");
	btnAddRow = new QPushButton("+ Добавить строку");

	// Стили для кнопок
	QString btnStyle = R"(
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
		})";

	btnToggleAll->setStyleSheet(btnStyle);
	btnRemoveRow->setStyleSheet(btnStyle);
	btnAddRow->setStyleSheet(btnStyle);
	btnConvert->setStyleSheet(btnStyle);

	btnLayout->addWidget(btnToggleAll);
	btnLayout->addWidget(btnConvert);
	btnLayout->addStretch(); // Отталкиваем кнопку добавления вниз (опционально)
	btnLayout->addWidget(btnAddRow);
	btnLayout->addWidget(btnRemoveRow);

	// Сборка главного layout
	mainLayout->addWidget(stackedWidget, 1); // 1 - коэффициент растяжения
	mainLayout->addLayout(btnLayout);

	// Подключение сигналов
	connect(btnToggleAll, &QPushButton::clicked, q, &KeyValueEditorWidget::onToggleAll);
	connect(btnConvert, &QPushButton::clicked, q, &KeyValueEditorWidget::onConvert);
	connect(btnAddRow, &QPushButton::clicked, q, &KeyValueEditorWidget::onAddRow);
	connect(btnRemoveRow, &QPushButton::clicked, q, &KeyValueEditorWidget::onRemoveSelectedRow);
}

void KeyValueEditorWidget::Private::addRow(const QString& param, const QString& value,
	const QString& desc, bool isEnabled) {
	int row = table->rowCount();
	table->insertRow(row);

	table->setItem(row, 0, new QTableWidgetItem(param));
	table->setItem(row, 1, new QTableWidgetItem(value));
	table->setItem(row, 2, new QTableWidgetItem(desc));

	// Настройка чекбокса для 4-й колонки
	QTableWidgetItem* checkItem = new QTableWidgetItem();
	checkItem->setCheckState(isEnabled ? Qt::Checked : Qt::Unchecked);
	checkItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	table->setItem(row, 3, checkItem);
}

void KeyValueEditorWidget::Private::removeSelectedRow() {
	auto selectionModel = table->selectionModel();
	auto selection = selectionModel->currentIndex();
	if (!selection.isValid()) {
		return;
	}

	table->removeRow(selection.row());
	selectionModel->clear();
}
