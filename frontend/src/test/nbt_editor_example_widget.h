#pragma once

#include <qlayout.h>

#include <components/mywidget.h>
#include <nbt_editor/nbt_editor_widget.h>

class NbtEditorExampleWidget : public MyWidget
{
public:
    explicit NbtEditorExampleWidget(QWidget* parent = nullptr)
        : MyWidget(parent)
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
