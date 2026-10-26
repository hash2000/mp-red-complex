#pragma once
#include "Libs/Resources/resources.h"
#include <QMainWindow>

class MainFrame	: public QMainWindow
{
  Q_OBJECT

public:
	MainFrame();
	virtual ~MainFrame() = default;
};
