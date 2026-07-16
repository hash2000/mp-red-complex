#pragma once
#include <QFontMetrics>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QFontMetrics>

namespace Extensions::TextEdit {
QTextEdit* setTabDistance(QTextEdit* obj, int distance);
QPlainTextEdit* setTabDistance(QPlainTextEdit* obj, int distance);
};
