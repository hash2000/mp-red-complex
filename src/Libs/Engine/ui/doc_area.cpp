#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/doc_widget.h"

#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

struct DocumentArea::Private {
	QStackedWidget* stack = nullptr;
	QWidget* emptyPage = nullptr;
	QList<DocumentWidget*> docs;
};

DocumentArea::DocumentArea(QWidget* parent)
	: QWidget(parent), d(std::make_unique<Private>()) {
	setObjectName(QStringLiteral("DocumentArea"));
	setAttribute(Qt::WA_StyledBackground);

	auto* lay = new QVBoxLayout(this);
	lay->setContentsMargins(0, 0, 0, 0);

	d->stack = new QStackedWidget(this);
	lay->addWidget(d->stack);

	d->emptyPage = new QWidget;
	d->emptyPage->setObjectName(QStringLiteral("EmptyPage"));
	d->emptyPage->setAttribute(Qt::WA_StyledBackground);
	auto* el = new QVBoxLayout(d->emptyPage);
	auto* hint = new QLabel(tr("Нет открытых документов\n\nCtrl+N — новый документ"), d->emptyPage);
	hint->setObjectName(QStringLiteral("EmptyHint"));
	hint->setAlignment(Qt::AlignCenter);
	el->addWidget(hint);
	d->stack->addWidget(d->emptyPage);

	connect(d->stack, &QStackedWidget::currentChanged, this, [this] {
		emit currentDocumentChanged(currentDocument());
	});
}

DocumentArea::~DocumentArea() {
	// ВАЖНО (ловушка PIMPL + Qt): дочерние виджеты удаляются в ~QWidget,
	// т.е. ПОСЛЕ уничтожения d. QStackedWidget при удалении страниц может
	// эмитить currentChanged -> наша лямбда обратится к уже удалённому d.
	d->stack->disconnect(this);
	for (auto* doc : d->docs)
		doc->disconnect(this);
	// Закрытые документы ждут deleteLater и до этого остаются дочерними
	// виджетами стека: их сигнал destroyed тоже нельзя доставлять сюда.
	for (auto* doc : d->stack->findChildren<DocumentWidget*>())
		doc->disconnect(this);
}

void DocumentArea::addDocument(DocumentWidget* doc) {
	if (!doc || d->docs.contains(doc))
		return;
	d->docs.append(doc);
	connect(doc, &QObject::destroyed, this, [this, doc] {
		if (!d->docs.removeOne(doc))
			return;
		emit documentDestroyed(doc);
		if (d->docs.isEmpty())
			d->stack->setCurrentWidget(d->emptyPage);
	});
	d->stack->addWidget(doc);
	emit documentAdded(doc);
	setCurrentDocument(doc);
}

bool DocumentArea::closeDocument(DocumentWidget* doc) {
	if (!doc || !d->docs.contains(doc))
		return true;

	if (doc->isModified())
		setCurrentDocument(doc); // показать пользователю, о чём спрашиваем

	if (!doc->close()) // -> closeEvent -> maybeClose()
		return false;

	const int idx = d->docs.indexOf(doc);
	const bool wasCurrent = (d->stack->currentWidget() == doc);

	emit documentAboutToClose(doc);
	d->docs.removeAt(idx);
	d->stack->removeWidget(doc);
	// removeWidget не меняет родителя: иначе закрытый документ останется
	// дочерним для стека и будет уничтожен вместе с ним после deleteLater.
	doc->setParent(nullptr);
	doc->deleteLater();                 // deleteLater: мы можем быть внутри его сигнала

	if (wasCurrent) {
		if (d->docs.isEmpty())
			d->stack->setCurrentWidget(d->emptyPage);
		else
			setCurrentDocument(d->docs.at(qMin(idx, int(d->docs.size()) - 1)));
	}

	return true;
}

bool DocumentArea::closeAll() {
	const auto docs = d->docs;
	for (auto* doc : docs)
		if (!closeDocument(doc))
			return false;
	return true;
}

void DocumentArea::setCurrentDocument(DocumentWidget* doc) {
	if (!doc || !d->docs.contains(doc)) return;
	d->stack->setCurrentWidget(doc);
	doc->setFocus();
}

DocumentWidget* DocumentArea::currentDocument() const {
	return qobject_cast<DocumentWidget*>(d->stack->currentWidget());
}

QList<DocumentWidget*> DocumentArea::documents() const {
	return d->docs;
}
