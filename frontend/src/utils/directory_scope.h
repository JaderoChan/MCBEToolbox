#pragma once

#include <qdir.h>
#include <qstring.h>

/** 临时切换当前工作目录的 RAII 辅助类，析构时自动恢复为构造前的原目录。 */
class DirectoryScope
{
public:
    explicit DirectoryScope(const QString& path)
    {
        originDir_ = QDir::currentPath();
        QDir::setCurrent(path);
    }

    ~DirectoryScope()
    {
        QDir::setCurrent(originDir_);
    }

private:
    QString originDir_;
};
