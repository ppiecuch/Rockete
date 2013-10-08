#ifndef DOCUMENTPREVIEW_H
#define DOCUMENTPREVIEW_H

#include <QMainWindow>

namespace Ui {
class DocumentPreview;
}

class DocumentPreview : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit DocumentPreview(QWidget *parent = 0);
    ~DocumentPreview();
    
private:
    Ui::DocumentPreview *ui;
};

#endif // DOCUMENTPREVIEW_H
