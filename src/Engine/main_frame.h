#pragma once
#include "Resources/resources.h"
#include "Base/config.h"
#include <QMainWindow>

class MainFrame
	: public QMainWindow
	, public Configurable
{
  Q_OBJECT

public:
	MainFrame();

	virtual ~MainFrame() = default;

	void configure(const std::shared_ptr<Config>& config) override;
};
