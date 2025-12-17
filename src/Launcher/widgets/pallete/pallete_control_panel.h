#pragma once
#include <QAbstractScrollArea>

class PalleteControlPanel: public QAbstractScrollArea {
  Q_OBJECT

public:
  explicit PalleteControlPanel(QWidget *parent = nullptr);
	virtual ~PalleteControlPanel() = default;


};
