#include "Launcher/main_frame/graphwidget/main_frame_test.h"
#include "Launcher/main_frame/graphwidget/graphwidget.h"
#include <QtWidgets>

MainFrameTest::MainFrameTest() {
	auto *widget = new GraphWidget;

	setCentralWidget(widget);
}
