#pragma once

#include <qlayout.h>

#include <components/trwidget.h>
#include <nbt_editor/nbt_editor_widget.h>

class NbtEditorExampleWidget : public TrWidget
{
public:
    explicit NbtEditorExampleWidget(QWidget* parent = nullptr)
        : TrWidget(parent)
    {
        widget_ = new NbtEditorWidget(this);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->addWidget(widget_);

        setWindowTitle("NBT Editor Example");
        resize(800, 600);
    }

private:
    NbtEditorWidget* widget_ = nullptr;
};
