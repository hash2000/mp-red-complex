#pragma once
#include "Libs/Resources/resources.h"
#include "Libs/Base/config.h"
#include <QMainWindow>

class MainFrame
	: public QMainWindow
	, public Configurable
{
  Q_OBJECT

public:
	MainFrame();

	virtual ~MainFrame() = default;

	void configure(const Config* config) override;
};
