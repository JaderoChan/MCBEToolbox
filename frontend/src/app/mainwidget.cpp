#include "mainwidget.h"

#include <qboxlayout.h>

#include "./examples/block_list_example_widget.h"

MainWidget::MainWidget(QWidget* parent)
    : TrWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new BlockListExampleWidget());
    resize(800, 600);
}
