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
        isOk_      = QDir::setCurrent(path);
    }

    ~DirectoryScope()
    {
        QDir::setCurrent(originDir_);
    }

    /** 如果切换目录失败返回 false（例如目标目录不存在），否则返回 true。 */
    bool isOk() const
    {
        return isOk_;
    }

private:
    bool    isOk_;
    QString originDir_;
};
