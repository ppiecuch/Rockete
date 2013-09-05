#ifndef ROCKETE_H
#define ROCKETE_H

#include <QtGui/QMainWindow>
#include <QFileSystemWatcher>
#include <QLabel>
#include "WizardButton.h"
#include "ui_rockete.h"
#include "RenderingView.h"
#include "OpenedDocument.h"
#include "OpenedStyleSheet.h"
#include "AttributeTreeModel.h"
#include "PropertyTreeModel.h"
#include "DocumentHierarchyEventFilter.h"


class QDRuler;
class Rockete : public QMainWindow
{
    Q_OBJECT

public:
    Rockete(QWidget *parent = 0, Qt::WFlags flags = 0);
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

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void changeEvent(QEvent *event); 
    virtual void closeEvent(QCloseEvent *event);

private:
    void newProject();
    void openProject(const QString &filePath);
    int openDocument(const char *);
    int openStyleSheet(const char *);
    int openLuaScript(const char *file_path);
    int openASCIIFile(const char *);
    void generateMenuRecent();
    void populateTreeView(const QString &top_item_name, const QString &directory_path);
    void loadPlugins();
    void closeTab(int index, bool must_save = true);
    void addRulers();

    Ui::rocketeClass ui;
    RenderingView *renderingView;
    AttributeTreeModel *attributeTreeModel;
    PropertyTreeModel *propertyTreeModel;
    QList<QAction*> recentFileActionList;
    QComboBox *searchBox;
    QComboBox *languageBox;
    QLabel *labelZoom;
    QFileSystemWatcher *fileWatcher;
    bool isReloadingFile;
    QMap<int, QString> fileChangedOutsideArray;
    WizardButton *wizard;
    QTreeWidgetItem *selectedTreeViewItem;
    DocumentHierarchyEventFilter *hierarchyEventFilter;
    QDRuler *mHorzRuler, *mVertRuler;

public:
    static Rockete *instance;
};

#define f_ssprintf(...)                                 \
    ({ int _ss_size = snprintf(0, 0, ##__VA_ARGS__);    \
    char *_ss_ret = (char*)alloca(_ss_size+1);          \
    snprintf(_ss_ret, _ss_size+1, ##__VA_ARGS__);       \
    _ss_ret; })

#define qInfo(...) { const char *b = ({ int _ss_size = snprintf(0, 0, ##__VA_ARGS__);   \
    char *_ss_ret = (char*)alloca(_ss_size+1);                                          \
    snprintf(_ss_ret, _ss_size+1, ##__VA_ARGS__);                                       \
    _ss_ret; }); Rockete::instance->logMessage(b); }

#endif // ROCKETE_H
