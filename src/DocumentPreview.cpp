#include "DocumentPreview.h"
#include "ui_preview.h"

DocumentPreview::DocumentPreview(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DocumentPreview)
{
    ui->setupUi(this);
}

DocumentPreview::~DocumentPreview()
{
    delete ui;
}
