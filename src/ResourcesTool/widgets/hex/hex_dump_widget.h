#pragma once
#include "ResourcesTool/widgets/base_tab_widget.h"
#include "DataStream/data_stream.h"
#include <QAbstractScrollArea>
#include <QByteArray>
#include <QMap>
#include <QTimer>

class HexDumpWidget : public BaseTabWidget {
public:
  explicit HexDumpWidget(std::shared_ptr<DataStream> stream, QWidget *parent = nullptr);
};
