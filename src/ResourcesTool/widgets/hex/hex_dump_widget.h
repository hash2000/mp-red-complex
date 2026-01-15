#pragma once
#include "BaseWidgets/tabs/base_tab_widget.h"
#include "Resources/resources.h"
#include "DataStream/data_stream.h"
#include <QAbstractScrollArea>
#include <QByteArray>
#include <QMap>
#include <QTimer>

class HexDumpWidget : public BaseTabWidget {
public:
  explicit HexDumpWidget(std::shared_ptr<Resources> resources, const QVariantMap& params, QWidget *parent = nullptr);
};
