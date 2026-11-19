#include "Libs/Engine/ui/name_select_dialog.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListView>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QKeyEvent>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QDebug>

class NameSelectDialog::Private {
public:
	QLineEdit* searchEdit = nullptr;
	QListView* listView = nullptr;
	QStringListModel* sourceModel = nullptr;
	QSortFilterProxyModel* proxyModel = nullptr;
	QString result;

	void setupUi(NameSelectDialog* q, const QStringList& variables);
};


void NameSelectDialog::Private::setupUi(NameSelectDialog* q, const QStringList& variables) {
	q->setWindowTitle("Выбор параметра");
	q->resize(400, 500);

	// Убираем рамку окна, если хотим стиль "попапа", 
	// но для надежности оставим стандартную рамку диалога или FramelessWindowHint
	// Для красоты можно добавить: q->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	// Но тогда нужно вручную обрабатывать закрытие по клику вне окна.
	// Оставим пока модальный диалог для простоты фокуса.

	auto* layout = new QVBoxLayout(q);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(6);

	// 1. Строка поиска
	searchEdit = new QLineEdit(q);
	searchEdit->setPlaceholderText("Поиск параметра...");
	searchEdit->setClearButtonEnabled(true);
	layout->addWidget(searchEdit);

	// 2. Модели
	sourceModel = new QStringListModel(variables, q);

	proxyModel = new QSortFilterProxyModel(q);
	proxyModel->setSourceModel(sourceModel);
	proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
	// Важно: FixedString означает поиск подстроки, а не Regex или Wildcard
	// В Qt6 это поведение по умолчанию для setFilterFixedString, 
	// но явно укажем, что нам нужно именно вхождение.
	proxyModel->setFilterKeyColumn(0);

	// 3. Список
	listView = new QListView(q);
	listView->setModel(proxyModel);
	listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	listView->setSelectionMode(QAbstractItemView::SingleSelection);
	// Убираем фокус с списка, чтобы он не перехватывал ввод текста, 
	// но оставляем возможность навигации стрелками
	listView->setFocusPolicy(Qt::StrongFocus);

	layout->addWidget(listView);

	// 4. Кнопки (Опционально, можно только по Enter/Double Click)
	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, q);
	layout->addWidget(buttonBox);

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, q, &QDialog::accept);
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, q, &QDialog::reject);

	// --- Логика ---

	// Фильтрация при вводе текста
	QObject::connect(searchEdit, &QLineEdit::textChanged, [this](const QString& text) {
		// setFilterFixedString ищет вхождение подстроки (substring match)
		proxyModel->setFilterFixedString(text);

		// Автоматически выделяем первый элемент после фильтрации, чтобы Enter сразу работал
		if (proxyModel->rowCount() > 0) {
			listView->setCurrentIndex(proxyModel->index(0, 0));
		}
	});

	// Выбор по двойному клику
	QObject::connect(listView, &QListView::doubleClicked, [this, q](const QModelIndex& index) {
		result = index.data().toString();
		q->accept();
	});

	// Выбор по Enter в списке
	QObject::connect(listView, &QListView::activated, [this, q](const QModelIndex& index) {
		result = index.data().toString();
		q->accept();
	});

	// Обработка Enter в строке поиска (выбрать текущий выделенный элемент)
	QObject::connect(searchEdit, &QLineEdit::returnPressed, [this, q]() {
		QModelIndex current = listView->currentIndex();
		if (current.isValid()) {
			result = current.data().toString();
			q->accept();
		}
	});

	// Навигация стрелками вниз из поиска в список
	q->installEventFilter(q); // Будет обрабатывать в eventFilter самого диалога? 
	// Нет, проще переопределить eventFilter в Private или использовать лямбду в самом диалоге.
	// Для чистоты сделаем обработку клавиш в самом классе диалога ниже.
}

NameSelectDialog::NameSelectDialog(const QStringList& variables, QWidget* parent)
	: QDialog(parent), d(std::make_unique<Private>()) {
	d->setupUi(this, variables);

	// Фокус на поиск при открытии
	d->searchEdit->setFocus();
}

NameSelectDialog::~NameSelectDialog() = default;

QString NameSelectDialog::selectedName() const {
	return d->result;
}

// Переопределяем keyPressEvent для удобной навигации
void NameSelectDialog::keyPressEvent(QKeyEvent* event) {
	// Если нажат Down в строке поиска -> переводим фокус на список
	if (d->searchEdit->hasFocus() && event->key() == Qt::Key_Down) {
		d->listView->setFocus();
		if (!d->listView->currentIndex().isValid() && d->proxyModel->rowCount() > 0) {
			d->listView->setCurrentIndex(d->proxyModel->index(0, 0));
		}

		return;
	}

	// Если нажат Up в списке и мы на первом элементе -> возвращаем фокус в поиск
	// Это сложнее отследить без кастомного ListView, но можно оставить стандартное поведение.

	// Стандартная обработка (Esc закрывает диалог)
	QDialog::keyPressEvent(event);
}
