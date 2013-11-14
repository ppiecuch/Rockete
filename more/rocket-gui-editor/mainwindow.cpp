/**********************************************************************************
 * License notice
 * ------------------------
 * mainwindow.cpp
 * ------------------------
 * Copyright (c) 2012 Alexander Kasper (alexander.limubei.kasper@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ***********************************************************************************/

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

#include <QFile>
#include <QTextCodec>
#include <QFileDialog>

#include <Rocket/Debugger.h>

CMainWindow::CMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CMainWindow)
{
    ui->setupUi(this);
    mHighlighter = new CRMLSyntaxHighlighter(ui->MainTextEdit->document());

    mGUIVizDock = new QDockWidget("GUI Viz", this);
    mGUIVizDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    mGUIVizDock->setWindowTitle("RocketViz");
    mRenderWidget = new CRenderWidget(ui->LogBrowser, ui->centralWidget);
    mGUIVizDock->setWidget(mRenderWidget);
    mGUIVizDock->setMinimumWidth(400);
    addDockWidget(Qt::RightDockWidgetArea, mGUIVizDock);

    // handle close events manually to just hide them
    mGUIVizDock->installEventFilter(this);
    ui->LogWidget->installEventFilter(this);

    connect(ui->actionLog, SIGNAL(toggled(bool)), this, SLOT(ToggleLog(bool)));
    connect(ui->actionVisualization, SIGNAL(toggled(bool)), this, SLOT(ToggleViz(bool)));
    connect(ui->actionShow_Hide_Debugger, SIGNAL(toggled(bool)), this, SLOT(ToggleDebugger(bool)));

    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(LoadRMLDocument()));
}

CMainWindow::~CMainWindow()
{
    mGUIVizDock->removeEventFilter(this);
    ui->LogWidget->removeEventFilter(this);
    delete ui;
    delete mHighlighter;
    delete mRenderWidget;
}

bool CMainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::Close)
    {
        if(obj == mGUIVizDock)
        {
            mGUIVizDock->hide();
            ui->actionVisualization->setChecked(false);
            return true;
        }
        else if(obj == ui->LogWidget)
        {
            ui->LogWidget->hide();
            ui->actionLog->setChecked(false);
            return true;
        }
        else return QObject::eventFilter(obj, event);
    }
    else return QObject::eventFilter(obj, event);
}

void CMainWindow::ToggleLog(bool toggle)
{
    if(toggle)
        ui->LogWidget->show();
    else
        ui->LogWidget->hide();
}

void CMainWindow::ToggleViz(bool toggle)
{
    if(toggle)
        mGUIVizDock->show();
    else
        mGUIVizDock->hide();
}

void CMainWindow::ToggleDebugger(bool toggle)
{
    if(toggle)
        Rocket::Debugger::SetVisible(true);
    else
        Rocket::Debugger::SetVisible(false);
}

void CMainWindow::LoadRMLDocument()
{
    QString FileName = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                                    QString(), tr("RML-Files (*.rml);;All Files (*)"));
    if (!QFile::exists(FileName))
    {
         Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR,
                                    "%s does not exist!", FileName.toStdString().c_str());
        return;
    }
    QFile File(FileName);
    if (!File.open(QFile::ReadOnly))
    {
        Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR,
                                   "Couldn't open file %s", FileName.toStdString().c_str());
        return;
    }

    QByteArray Data = File.readAll();
    QTextCodec *Codec = Qt::codecForHtml(Data);
    QString Str = Codec->toUnicode(Data);
    Str = QString::fromLocal8Bit(Data);
    ui->MainTextEdit->setPlainText(Str);
    while(ui->mTextTabs->count() > 1)
        ui->mTextTabs->removeTab(1);
    ui->mTextTabs->setCurrentIndex(0);
    mCurrentDir = QDir(FileName);
    ui->mTextTabs->setTabText(0, mCurrentDir.dirName());

    if(mRenderWidget->LoadDocument(Str, FileName))
    {
        // find RCSS links and load these as well
        QString RCSSLinkTag = "text/rcss";

        int Idx = Str.indexOf(RCSSLinkTag);
        while(Idx >= 0 && Idx < Str.length())
        {
            QString RCSSFileName = Str.mid(Str.indexOf("href=", Idx)+6, Str.indexOf("/>", Idx)-Str.indexOf("href=", Idx)-7);

            LoadRCSS(RCSSFileName);
            Idx = Str.indexOf(RCSSLinkTag, Idx+RCSSLinkTag.length());
        }
    }
}

void CMainWindow::LoadRCSS(QString FilePath)
{
    QString AbsFilePath = QFileInfo(mCurrentDir.absolutePath()).absoluteDir().absolutePath() + "/" + FilePath;
    if(!QFile::exists(AbsFilePath))
    {
         Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR,
                                    "%s does not exist!", AbsFilePath.toStdString().c_str());
        return;
    }

    QFile File(AbsFilePath);
    if (!File.open(QFile::ReadOnly))
    {
        Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR,
                                   "Couldn't open file %s", AbsFilePath.toStdString().c_str());
        return;
    }

    QByteArray Data = File.readAll();
    QTextCodec *Codec = Qt::codecForHtml(Data);
    QString Str = Codec->toUnicode(Data);
    Str = QString::fromLocal8Bit(Data);

    QTextEdit* RCSSWidget = new QTextEdit(this);
    RCSSWidget->setPlainText(Str);
    QDir tmp(FilePath);
    ui->mTextTabs->addTab(RCSSWidget, tmp.dirName());
}
