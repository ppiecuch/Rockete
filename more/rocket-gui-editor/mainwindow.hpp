/**********************************************************************************
 * License notice
 * ------------------------
 * mainwindow.hpp
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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QDir>

#include "rmlsyntaxhighlighter.hpp"
#include "renderwidget.hpp"

namespace Ui {
class CMainWindow;
}

class CMainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit CMainWindow(QWidget *parent = 0);
    ~CMainWindow();
    
private:
    Ui::CMainWindow *ui;

    CRMLSyntaxHighlighter* mHighlighter;

    CRenderWidget* mRenderWidget;
    QDockWidget* mGUIVizDock;

    QDir mCurrentDir;

    void LoadRCSS(QString FilePath);

    bool eventFilter(QObject* obj, QEvent* event);

private slots:
    void LoadRMLDocument();
    void ToggleLog(bool);
    void ToggleViz(bool);
    void ToggleDebugger(bool);
};

#endif // MAINWINDOW_HPP
