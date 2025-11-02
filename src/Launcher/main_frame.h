#pragma once
#include "Launcher/config.h"
#include <QMainWindow>

class MainFrame
	: public QMainWindow
	, public Configured
{
  Q_OBJECT

public:
	MainFrame();

	virtual ~MainFrame() = default;

	void configure(const Config& config) override;
};
