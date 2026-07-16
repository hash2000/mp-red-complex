#include "Content/BaseWidgets/key_value/key_value_editor_widget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
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
	QToolButton* btnToggleAll;
	QToolButton* btnConvert;
	QToolButton* btnRemoveRow;
	QToolButton* btnAddRow;

	void setupUi();
	void addRow(const QString& param = "", const QString& value = "",
		const QString& desc = "", bool isEnabled = true);
	void removeSelectedRow();
	void setTableReadOnly(bool set = true);

	QToolButton* addToolButton(const QString& title, const QString& tooltip);
};

KeyValueEditorWidget::KeyValueEditorWidget(QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {
	d->setupUi();
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
		QString text;
		for (int i = 0; i < d->table->rowCount(); ++i) {
			QString param = d->table->item(i, 0)->text().trimmed();
			QString value = d->table->item(i, 1)->text();
			QString desc = d->table->item(i, 2)->text().trimmed();
			bool isEnabled = d->table->item(i, 3)->checkState() == Qt::Checked;

			// Пропускаем полностью пустые строки
			if (param.isEmpty() && value.trimmed().isEmpty()) {
				continue;
			}

			// Формируем строку: [// ]key: value [<p>desc</p>]
			QString line = (isEnabled ? "" : "// ");
			line += param + ": " + value;
			if (!desc.isEmpty()) {
				line += " <p>" + desc + "</p>";
			}
			text += line + "\n";
		}
		d->textEdit->setPlainText(text);
		d->stackedWidget->setCurrentIndex(1);
		d->btnConvert->setText("⌗");
	}
	else {
		// ==========================================
		// Режим: Текст -> Таблица
		// ==========================================
		QString text = d->textEdit->toPlainText();

		// Регулярное выражение с поддержкой многострочности (MultilineOption)
		// Группа 1: (//\s*)?      - опциональный комментарий и пробелы
		// Группа 2: ([^:\n]*?)    - имя параметра (все символы до первого ':', не включая перенос строки)
		// Группа 3: (.*?)         - значение (все до <p> или конца строки)
		// Группа 4: (.*?)         - описание внутри <p>...</p> (опционально)
		QRegularExpression re(
			"^(//\\s*)?([^:\\n]*?)\\s*:\\s*(.*?)(?:\\s*<p>(.*?)</p>)?\\s*$",
			QRegularExpression::MultilineOption
		);

		QRegularExpressionMatchIterator it = re.globalMatch(text);
		d->table->setRowCount(0); // Очищаем таблицу

		while (it.hasNext()) {
			QRegularExpressionMatch match = it.next();

			QString param = match.captured(2).trimmed();
			QString value = match.captured(3).trimmed();
			QString desc = match.captured(4).trimmed();

			// Если есть группа 1 ("//"), значит параметр выключен
			bool isEnabled = match.captured(1).isEmpty();

			d->addRow(param, value, desc, isEnabled);
		}

		// Добавляем одну пустую строку в конец для удобства
		if (d->table->rowCount() == 0) {
			d->addRow();
		}
		else {
			// Проверяем, пустая ли последняя строка, если нет - добавляем новую
			bool lastRowEmpty =
				d->table->item(d->table->rowCount() - 1, 0)->text().isEmpty() &&
				d->table->item(d->table->rowCount() - 1, 1)->text().isEmpty();
			if (!lastRowEmpty) {
				d->addRow();
			}
		}

		d->stackedWidget->setCurrentIndex(0);
		d->btnConvert->setText("🖹");
	}
}

void KeyValueEditorWidget::onAddRow() {
	d->addRow();
}

void KeyValueEditorWidget::onRemoveSelectedRow() {
	d->removeSelectedRow();
}

QToolButton* KeyValueEditorWidget::Private::addToolButton(const QString& title, const QString& tooltip) {
	auto btn = new QToolButton(q);
	btn->setText(title);
	btn->setToolTip(tooltip);
	btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
	btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	return btn;
}

void KeyValueEditorWidget::Private::setupUi() {
	QHBoxLayout* mainLayout = new QHBoxLayout(q);
	mainLayout->setContentsMargins(10, 10, 10, 10);

	// --- ЛЕВАЯ ЧАСТЬ: Таблица и Текст ---
	stackedWidget = new QStackedWidget();

	// 1. Таблица
	table = new QTableWidget(0, 4);
	table->setHorizontalHeaderLabels({ "Имя", "Значение", "Описание", "Вкл/Выкл" });
	table->verticalHeader()->setVisible(true);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);

	// 2. Текстовый редактор
	textEdit = new QPlainTextEdit();
	QFont monoFont("Consolas", 8);
	if (!monoFont.exactMatch()) {
		monoFont = QFont("Courier New", 8);
	}
	textEdit->setFont(monoFont);

	stackedWidget->addWidget(table);
	stackedWidget->addWidget(textEdit);

	// --- ПРАВАЯ ЧАСТЬ: Кнопки ---
	QVBoxLayout* btnLayout = new QVBoxLayout();
	btnLayout->setSpacing(10);

	btnToggleAll = addToolButton("☒", "Вкл/Выкл все параметры");
	btnConvert = addToolButton("🖹", "Строковый/Табличный вид");
	btnRemoveRow = addToolButton("-", "Удалить параметр");
	btnAddRow = addToolButton("+", "Добавить параметр");

	btnLayout->addWidget(btnToggleAll);
	btnLayout->addWidget(btnConvert);
	btnLayout->addStretch();
	btnLayout->addWidget(btnAddRow);
	btnLayout->addWidget(btnRemoveRow);

	// Сборка главного layout
	mainLayout->addWidget(stackedWidget, 1);
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

std::map<QString, QString> KeyValueEditorWidget::parametersMap(bool enabledOnly) const {
	const auto count = d->table->rowCount();
	std::map<QString, QString> result;
	for (int i = 0; i < count; i++) {
		const auto name = d->table->item(i, 0)->text();
		const auto value = d->table->item(i, 1)->text();
		const auto isEnabled = d->table->item(i, 3)->checkState() != Qt::Unchecked;
		const bool skipByNameOrValue = name.isEmpty() || value.isEmpty();
		const bool skipByEnabled = enabledOnly && !isEnabled;
		if (skipByNameOrValue || skipByEnabled) {
			continue;
		}

		result.emplace(name, value);
	}

	return std::move(result);
}

std::vector<KeyValueEditorWidget::Parameter> KeyValueEditorWidget::parameters() const {
	const auto count = d->table->rowCount();
	std::vector<KeyValueEditorWidget::Parameter> result;
	result.resize(count);
	for (int i = 0; i < count; i++) {
		auto& param = result[i];
		param.name = d->table->item(i, 0)->text();
		param.value = d->table->item(i, 1)->text();
		param.description = d->table->item(i, 2)->text();
		param.isEnabled = d->table->item(i, 3)->checkState() != Qt::Unchecked;
	}
	return std::move(result);
}

void KeyValueEditorWidget::setParameters(const std::vector<KeyValueEditorWidget::Parameter>& params) {
	d->table->setRowCount(0);
	applyParameters(params);
}

void KeyValueEditorWidget::applyParameters(const std::vector<KeyValueEditorWidget::Parameter>& params) {
	for (const auto& it : params) {
		d->addRow(it.name, it.value, it.description, it.isEnabled);
	}
}

void KeyValueEditorWidget::setReadonly(bool set) {
	d->btnToggleAll->setDisabled(set);
	d->btnRemoveRow->setDisabled(set);
	d->btnAddRow->setDisabled(set);
	d->textEdit->setReadOnly(set);
	d->setTableReadOnly(set);
}

void KeyValueEditorWidget::Private::setTableReadOnly(bool set) {
	if (set) {
		// Запрещаем редактирование через UI (двойной клик, клавиши)
		table->setEditTriggers(QAbstractItemView::NoEditTriggers);

		// Для всех элементов устанавливаем read-only флаги
		for (int i = 0; i < table->rowCount(); ++i) {
			for (int j = 0; j < table->columnCount(); ++j) {
				QTableWidgetItem* item = table->item(i, j);
				if (item) {
					// Qt::ItemIsSelectable - позволяет выделять для копирования
					// Qt::ItemIsEnabled - отображает элемент нормально
					// БЕЗ Qt::ItemIsEditable - запрещает редактирование
					// БЕЗ Qt::ItemIsUserCheckable - запрещает клики по чекбоксу
					item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
				}
			}
		}
	}
	else {
		// Восстанавливаем возможность редактирования
		table->setEditTriggers(QAbstractItemView::DoubleClicked |
			QAbstractItemView::EditKeyPressed |
			QAbstractItemView::AnyKeyPressed);

		for (int i = 0; i < table->rowCount(); ++i) {
			for (int j = 0; j < table->columnCount(); ++j) {
				QTableWidgetItem* item = table->item(i, j);
				if (item) {
					if (j == 3) { // Колонка чекбоксов
						// Возвращаем возможность кликать по чекбоксу
						item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
					}
					else {
						// Возвращаем возможность редактирования
						item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
					}
				}
			}
		}
	}
}
