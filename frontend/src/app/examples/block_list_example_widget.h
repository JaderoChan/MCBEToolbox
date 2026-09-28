#pragma once

#include <qboxlayout.h>

#include <block.hpp>
#include <config.h>
#include <utils/directory_scope.h>
#include "../block_list_widget.h"
#include "../trwidget.h"

class BlockListExampleWidget : public TrWidget
{
public:
    explicit BlockListExampleWidget(QWidget* parent = nullptr)
        : TrWidget(parent)
    {
        DirectoryScope dirScope(APP_RESOURCES_DIRPATH);
        entries_ = parseBlockEntriesFromFile(APP_BLOCK_ENTRIES_FILEPATH);

        widget_ = new BlockListWidget(entries_);
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->addWidget(widget_);
    }

private:
    BlockEntryMap    entries_;
    BlockListWidget* widget_ = nullptr;
};
