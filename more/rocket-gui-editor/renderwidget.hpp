/**********************************************************************************
 * License notice
 * ------------------------
 * renderwidget.hpp
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

#ifndef RENDERWIDGET_HPP
#define RENDERWIDGET_HPP

#include <QGLWidget>
#include <QTimer>

#include <Rocket/Core.h>
#include "ShellRenderInterfaceOpenGL.h"
#include "qtsysteminterface.hpp"

class QTextEdit;

class CRenderWidget : public QGLWidget
{
    Q_OBJECT
public:
    explicit CRenderWidget(QTextEdit* LogWidget, QWidget *parent = 0);
    ~CRenderWidget();

    bool LoadDocument(QString DocString, QString URL);
    
protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);


private:
    Rocket::Core::Context* mGUIContext;
    ShellRenderInterfaceOpenGL* mRenderInterface;
    CQtSystemInterface* mSystemInterface;

    Rocket::Core::ElementDocument* mCurrentDoc;

    typedef std::map<Qt::Key, Rocket::Core::Input::KeyIdentifier> KeyIdentifierMapType;
    KeyIdentifierMapType mKeyIdentifierMap;

    QTimer mRenderTimer;

    void BuildKeyMap();
};

#endif // RENDERWIDGET_HPP
