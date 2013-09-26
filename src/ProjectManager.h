#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QDebug>
#include <QDomDocument>
#include "Settings.h"

struct CssCuttingInfo {
    int left, top, right, bottom;
    CssCuttingInfo() : left(0), top(0), right(0), bottom(0) { }
    CssCuttingInfo(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) { }
    CssCuttingInfo(const QString &inf) { int n = sscanf(inf.toLatin1().constData(), "%d;%d;%d;%d", &left, &top, &right, &bottom); if (n!=4)
        qDebug() << "Invalid cutting data: " << inf;
    }
    operator const QString() const { return QString("%1;%2;%3;%4").arg(left).arg(top).arg(right).arg(bottom); }
    bool isEmpty() const { return (left == 0 && top == 0 && right == 0 && bottom == 0); }
};

class ProjectManager
{
public:
    ProjectManager();
    ~ProjectManager();

    static ProjectManager & getInstance() {
        static ProjectManager instance;
        return instance;
    }

    bool Initialize(const QString &filename);
    void Serialize(const QString &filename);

    const QStringList &getFontPaths(){return fontPaths;}
    const QStringList &getTexturePaths(){return texturePaths;}
    const QStringList &getInterfacePaths(){return interfacePaths;}
    const QString &getWordListPath(){return wordListsPath;}
    const QString &getSnippetsFolderPath(){return snippetsFolderPath;}
    const QString &getLocalizationOpeningTag(){return localizationOpeningTag;}
    const QString &getLocalizationClosingTag(){return localizationClosingTag;}
    void setCuttingInfo(const QString &key, const CssCuttingInfo &info);
    CssCuttingInfo getCuttingInfo(const QString &key);
private:
    QString projectFile, projectName;
    QStringList fontPaths;
    QStringList texturePaths;
    QStringList interfacePaths;
    QString wordListsPath;
    QString snippetsFolderPath;
    QString localizationOpeningTag;
    QString localizationClosingTag;
    QMap<QString, CssCuttingInfo> cutting;
};
#endif
