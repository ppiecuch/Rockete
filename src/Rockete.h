#ifndef ROCKETE_H
#define ROCKETE_H

#include <stdarg.h>
#include <QMainWindow>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QTemporaryFile>
#include "WizardButton.h"
#include "ui_rockete.h"
#include "RenderingView.h"
#include "OpenedDocument.h"
#include "OpenedStyleSheet.h"
#include "AttributeTreeModel.h"
#include "PropertyTreeModel.h"
#include "DocumentHierarchyEventFilter.h"

class QDRuler;
class QDLabel;

struct LocalScreenSizeItem
{
    LocalScreenSizeItem() : width(0), height(0)
    {
    }

    LocalScreenSizeItem(const int _width, const int _height, const char *text=NULL)
    {
        width = _width;
        height = _height;
        displayedString = QString::number(width) + "x" + QString::number(height);
        displayedTwoString = displayedString;

        if (text)
        {
            labelString = text;
            displayedString += " (";
            displayedString += text;
            displayedString += ")";
            displayedTwoString += "\n(";
            displayedTwoString += text;
            displayedTwoString += ")";
        }
    }

    QString labelString;
    QString displayedString;
    QString displayedTwoString; // two line label
    int width;
    int height;
};

// available frames:
extern struct TestFrameInfo {
    const char *image;
    bool tool, toolbar;
    LocalScreenSizeItem size;
} testFrames[];

class Rockete : public QMainWindow
{
    Q_OBJECT

    friend void logMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &amsg );

public:
    Rockete(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~Rockete();
    static Rockete & getInstance() {
        return *instance;
    }
    void repaintRenderingView();
    void fillAttributeView();
    void fillPropertyView();
    void selectElement(Element *element);
    void reloadCurrentDocument();
    int getTabIndexFromFileName(const char * name); // TODO: clean up this... its a bit dirty. At least use full path.
    OpenedDocument *getCurrentTabDocument(int index = -1);
    OpenedStyleSheet *getCurrentTabStyleSheet(int index = -1);
    OpenedFile *getOpenedFile(const char * file_path, const bool try_to_open=false);
    //OpenedFile *getOpenedFileFromTabIndex(const int tab_index);
    QWidget *getCurrentToolTab() { return ui.currentToolTab; }
    OpenedDocument *getCurrentDocument() { return renderingView->getCurrentDocument(); }
    QString getPathForFileName(const QString &filename);
    void setZoomLevel(float level);
    QFileSystemWatcher *getFileWatcher(){return fileWatcher;}
    int openFile(const QString &file_path);
    void checkTextChanged(int index);
    SnippetsManager *getSnippetsManager(){return ui.snippetsListWidget;}

    void logMessage(QString aMsg);

public slots:
    void menuOpenClicked();
    void menuNewProjectClicked();
    void menuOpenProjectClicked();
    void menuSaveProjectClicked();
    void menuReloadProjectClicked();
    void menuSaveClicked();
    void menuSaveAsClicked();
    void menuCloseClicked();
    void codeTextChanged();
    void codeTabChanged(int index);
    void codeTabRequestClose(int index);
    void unselectElement();
    void menuReloadClicked();
    void menuSetScreenSizeClicked();
    void menuLoadFonts();
    void menuUndoClicked();
    void menuRedoClicked();
    void menuReloadAssetsClicked();
    void menuFormatTextClicked();
    void propertyViewClicked(const QModelIndex &index);
    void attributeViewClicked(const QModelIndex &index);
    void menuRecentFileClicked(QAction *action);
    void menuBackgroundChangeColor();
    void menuBackgroundChangeImage();
    void languageBoxActivated();
    void newButtonWizardActivated();
    void fileTreeDoubleClicked(QTreeWidgetItem *item, int column);
    void fileTreeClicked(QTreeWidgetItem *item, int column);
    void documentHierarchyDoubleClicked(QTreeWidgetItem *item, int column);
    void fileHasChanged(const QString &path);
    void findTriggered();
    void findPreviousTriggered();
    void findNextTriggered();
    void replaceTriggered();
    void replaceAllTriggered();
    void replaceAllInAllTriggered();
    void addSnippetClicked();
    void removeSnippetClicked();
    void snippetsListDoubleClicked(QListWidgetItem *item);
    void resizeTexturePreview(QResizeEvent * event);
    void resizeCuttingPreview(QResizeEvent * event);
    void spinCuttingChanged(int value);
    void cuttingPrevSizeChanged(int value);
    void cuttingMaskToggle(bool value);
    void cuttingPreviewTabChange(int tab);
    void splitterMovedChanges(int pos, int index);
    void newScreenSizeAction();
    void orientationChange(QAction *action);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void changeEvent(QEvent *event); 
    virtual void closeEvent(QCloseEvent *event);

private:
    void newProject();
    void openProject(const char *, bool = false);
    void saveProject(const char *);
    int openDocument(const char *);
    int openStyleSheet(const char *);
    int openLuaScript(const char *);
    int openASCIIFile(const char *);
    void generateMenuRecent();
    QString readSpriteSheetInfo(QTreeWidgetItem *item, const QString &texture);
    void populateTreeView(const QString &top_item_name, const QString &directory_path);
    void loadPlugins();
    void closeTab(int index, bool must_save = true);
    void addRulers();
    void updateCuttingTab(const QString &file, const QString &texture, int l, int b, int w, int h);
    void updateCuttingInfo(int lvalue, int tvalue, int rvalue, int bvalue);
    bool updateTextureInfoFiles();  // true - new file(s) has been created
    bool updateTextureInfoFiles(const QString &force);
    void setScreenSize(int width, int height);
    // open preview window with document size w x h
    void openPreviewWindow(int w, int h);

    void logHtmlMessage(QString aMsg);

    Ui::rocketeClass ui;
    RenderingView *renderingView;
    AttributeTreeModel *attributeTreeModel;
    PropertyTreeModel *propertyTreeModel;
    QList<QAction*> recentFileActionList;
    QComboBox *searchBox;
    QComboBox *languageBox;
    QLabel *labelZoom, *labelPos, *labelCuttingMask, *labelScreenSize;
    QFileSystemWatcher *fileWatcher;
    bool isReloadingFile;
    QMap<int, QString> fileChangedOutsideArray;
    WizardButton *wizard;
    QTreeWidgetItem *selectedTreeViewItem;
    DocumentHierarchyEventFilter *hierarchyEventFilter;
    QDRuler *horzRuler, *vertRuler;
    QImage selectedTexture; // selected texture image/subimage
    QTemporaryFile selectedTextureTmp; // selected texture tmp. image file
    QMap<QString, QMap<QString, QRect> > texturesAtlasInf; // all textures/atlas info
    // opened project file
    QString projectFile;

public:
    static Rockete *instance;
};

#if defined(Q_CC_MSVC) || defined(Q_CC_MSVC_NET)
inline char *f_ssprintf(const char *format, va_list args) {
   int _ss_size = _vsnprintf(0, 0, format, args);
   char *_ss_ret = (char*)alloca(_ss_size+1);
   _vsnprintf(_ss_ret, _ss_size+1, format, args);
   return _ss_ret;
}

# define qInfo(...) do { int _ss_size = _snprintf(0, 0, ##__VA_ARGS__);    \
    char *_ss_ret = (char*)alloca(_ss_size+1);                          \
    _snprintf(_ss_ret, _ss_size+1, ##__VA_ARGS__);                      \
    Rockete::instance->logMessage(_ss_ret); } while(0)

#else

# define f_ssprintf(...)                                \
    ({ int _ss_size = snprintf(0, 0, ##__VA_ARGS__);    \
    char *_ss_ret = (char*)alloca(_ss_size+1);          \
    snprintf(_ss_ret, _ss_size+1, ##__VA_ARGS__);       \
    _ss_ret; })

# define qInfo(...) do { const char *b = ({ int _ss_size = snprintf(0, 0, ##__VA_ARGS__);  \
    char *_ss_ret = (char*)alloca(_ss_size+1);                                          \
    snprintf(_ss_ret, _ss_size+1, ##__VA_ARGS__);                                       \
    _ss_ret; }); Rockete::instance->logMessage(b); } while(0)
#endif
#endif // ROCKETE_H
