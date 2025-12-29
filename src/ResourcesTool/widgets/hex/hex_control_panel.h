#pragma once
#include "DataStream/data_stream.h"
#include <QWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <memory>

class HexDumpWidget;

class HexControlPanel : public QWidget {
  Q_OBJECT

public:
  explicit HexControlPanel(HexDumpWidget *hexWidget, std::shared_ptr<DataStream> stream, QWidget *parent = nullptr);

  void updateFromSelection();

signals:
  void selectionChanged(qsizetype offset, qsizetype length);

private slots:
  void onUpdateSelection();
	void onNextBytesGroup();
	void onPrevBytesGroup();
	void onHexSelectionChanged(qint64 offset, qint64 length, const QByteArray& selected);

private:
  HexDumpWidget *_hexWidget;
	std::shared_ptr<DataStream> _stream;

  QSpinBox *_offsetSpinBox = nullptr;
  QSpinBox *_lengthSpinBox = nullptr;
  QLineEdit *_valueEdit = nullptr;
  QLineEdit *_valueUnsignedEdit = nullptr;
	QPushButton *_nextButton = nullptr;
	QPushButton *_prevButton = nullptr;
};
