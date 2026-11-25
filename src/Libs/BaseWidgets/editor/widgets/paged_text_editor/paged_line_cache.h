#pragma once
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

/// Кэш страниц текста для PagedTextEditor.
///
/// Хранит только несколько последних страниц; строки читаются с диска
/// по требованию. Не знает о виджете, курсоре и поиске.
class PagedLineCache {
public:
	PagedLineCache(int pageLines = 2000, int maxCachedPages = 4);

	/// Сбросить кэш и привязать его к файлу и индексу строк.
	void reset(const QString& path, const QByteArray& encodingName,
		const QVector<qint64>& lineStarts, qint64 fileSize);
	void clear();

	QString lineAt(int line) const;
	int pageLines() const;

private:
	struct Page {
		QStringList lines;
	};

	void loadPage(int line) const;

	QString path;
	QByteArray encodingName;
	QVector<qint64> lineStarts;
	qint64 fileSize = 0;
	int linesPerPage = 2000;
	int maxCachedPages = 4;
	mutable QHash<int, Page> pages;
	mutable QList<int> pageOrder;
};
