#pragma once

#include <qboxlayout.h>

#include <mcnbt/mcnbt.hpp>

#include <components/trwidget.h>
#include <nbt_editor/nbt_tree_widget.h>

class NbtTreeExampleWidget : public TrWidget
{
public:
    explicit NbtTreeExampleWidget(QWidget* parent = nullptr)
        : TrWidget(parent)
    {
        widget_ = new NbtTreeWidget();
        auto tag = generateExampleNBT();
        widget_->nbtView()->setRootName("Sample");
        widget_->nbtView()->setRootTag(tag);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->addWidget(widget_);

        setWindowTitle("NBT Tree Example");
        resize(800, 600);
    }

private:
    nbt::Tag generateExampleNBT()
    {
        nbt::Tag player = nbt::Tag::compound();
        player["Name"]       = "Steve";
        player["Health"]     = 20;
        player["Speed"]      = 2.0f;
        player["IsCreative"] = false;

        nbt::Tag position = nbt::Tag::list(nbt::TT_DOUBLE);
        position.getList().resize(3);
        position[0] = 100.0;
        position[1] = -60.0;
        position[2] = 100.0;
        player["Pos"] = std::move(position);

        nbt::Tag inventory = nbt::Tag::list(nbt::TT_COMPOUND);
        {
            nbt::Tag item = nbt::Tag::compound();
            item["Id"]    = "minecraft:dirt";
            item["Count"] = 64;
            inventory.pushBack(std::move(item));
        }
        {
            nbt::Tag item = nbt::Tag::compound();
            item["Id"]    = "minecraft:iron_sword";
            item["Count"] = 1;
            nbt::Tag enchantments = nbt::Tag::list(nbt::TT_COMPOUND);
            {
                nbt::Tag enchantment = nbt::Tag::compound();
                enchantment["Id"]    = "minecraft:sharp";
                enchantment["level"] = 3;
                enchantments.pushBack(std::move(enchantment));
            }
            {
                nbt::Tag enchantment = nbt::Tag::compound();
                enchantment["Id"]    = "minecraft:durability";
                enchantment["level"] = 1;
                enchantments.pushBack(std::move(enchantment));
            }
            item["enchantments"] = std::move(enchantments);
            inventory.pushBack(std::move(item));
        }
        player["Inventory"] = std::move(inventory);

        nbt::Tag root = nbt::Tag::compound();
        root["Version"] = 1;
        root["Player"]  = std::move(player);

        return root;
    }

    NbtTreeWidget* widget_ = nullptr;
};
