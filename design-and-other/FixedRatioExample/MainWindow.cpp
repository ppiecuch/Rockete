#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "QSingleItemSquareLayout.h"
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QWidget* centralWidget = new QWidget;
    setCentralWidget(centralWidget);
    QSingleItemSquareLayout* mainLayout = new QSingleItemSquareLayout;

    QTextEdit* editor = new QTextEdit;
    mainLayout->addWidget(editor);

    centralWidget->setLayout(mainLayout);
}

MainWindow::~MainWindow()
{
    delete ui;
}
