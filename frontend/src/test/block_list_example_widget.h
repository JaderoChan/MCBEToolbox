#pragma once

#include <qboxlayout.h>

#include <block.hpp>
#include <config.h>
#include <app/block_list_widget.h>
#include <components/mywidget.h>
#include <utils/directory_scope.h>

class BlockListExampleWidget : public MyWidget
{
public:
    explicit BlockListExampleWidget(QWidget* parent = nullptr)
        : MyWidget(parent)
    {
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        entries_ = parseBlockEntriesFromFile(APP_BLOCK_ENTRIES_FILEPATH);
        widget_ = new BlockListWidget(entries_);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->addWidget(widget_);

        setWindowTitle("Block List Example");
        resize(800, 600);
    }

private:
    BlockEntryMap    entries_;
    BlockListWidget* widget_ = nullptr;
};
