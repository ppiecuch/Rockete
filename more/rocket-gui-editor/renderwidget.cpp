/**********************************************************************************
 * License notice
 * ------------------------
 * renderwidget.cpp
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

#include "renderwidget.hpp"

#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>
#include <Rocket/Core/StreamMemory.h>

#include <QDir>
#include <QMouseEvent>
#include <QKeyEvent>

namespace RC = Rocket::Core;

namespace {
int GetKeyModifierState(Qt::KeyboardModifiers km)
{
    int mod_state = 0;

    if(km.testFlag(Qt::ControlModifier))
        mod_state |= Rocket::Core::Input::KM_CTRL;
    if(km.testFlag(Qt::ShiftModifier))
        mod_state |= Rocket::Core::Input::KM_SHIFT;
    if(km.testFlag(Qt::AltModifier))
        mod_state |= Rocket::Core::Input::KM_ALT;

    return mod_state;
}
}

CRenderWidget::CRenderWidget(QTextEdit* LogWidget, QWidget* parent) :
    QGLWidget(parent),
    mRenderInterface(new ShellRenderInterfaceOpenGL()),
    mSystemInterface(new CQtSystemInterface(LogWidget)),
    mCurrentDoc(NULL)
{
    setMouseTracking(true);
    BuildKeyMap();

    Rocket::Core::SetRenderInterface(mRenderInterface);
    Rocket::Core::SetSystemInterface(mSystemInterface);

    Rocket::Core::Initialise();
    Rocket::Controls::Initialise();
    Rocket::Core::Log::Initialise();

    RC::Log::Message(RC::Log::LT_ERROR, "Test error msg");
    RC::Log::Message(RC::Log::LT_WARNING, "Test warning msg");

    QDir WorkingDir = QDir::current();
    RC::Log::Message(RC::Log::LT_INFO, "Loading fonts from %s",
                  WorkingDir.absolutePath().toStdString().c_str());
    WorkingDir.setFilter(QDir::Files);
    QStringList FileTypes;
    FileTypes.append(QString("*.otf"));
    QStringList List = WorkingDir.entryList(FileTypes);
    foreach(const QString& F, List)
    {
        Rocket::Core::String FontFile(F.toStdString().c_str());
        Rocket::Core::FontDatabase::LoadFontFace(FontFile);
    }

    RC::Log::Message(RC::Log::LT_INFO, "Creating Rocket context...");
    Rocket::Core::Vector2i size(width(), height());
    mGUIContext = Rocket::Core::CreateContext("Main", size);

    Rocket::Debugger::Initialise(mGUIContext);
    Rocket::Debugger::SetVisible(true);
    RC::Log::Message(RC::Log::LT_INFO, "done!\n");

    connect(&mRenderTimer, SIGNAL(timeout()), this, SLOT(update()));
    mRenderTimer.start();
}

CRenderWidget::~CRenderWidget()
{
    mRenderTimer.stop();

    Rocket::Core::Log::Shutdown();

    if(mCurrentDoc != NULL)
    {
        mCurrentDoc->Close();
        mCurrentDoc->RemoveReference();
        mGUIContext->UnloadAllDocuments();
    }

    mGUIContext->RemoveReference();

    Rocket::Core::Shutdown();

    delete mRenderInterface;
    delete mSystemInterface;
}

bool CRenderWidget::LoadDocument(QString DocString, QString URL)
{
    if(mCurrentDoc)
    {
        mCurrentDoc->Close();
        mCurrentDoc->RemoveReference();
        mGUIContext->UnloadDocument(mCurrentDoc);
    }

    Rocket::Core::String fn(DocString.toAscii());
    Rocket::Core::String URLStr(URL.replace(':', '|').toAscii());

    Rocket::Core::StreamMemory* DocStream = new Rocket::Core::StreamMemory((Rocket::Core::byte*)fn.CString(), fn.Length());
    DocStream->SetSourceURL(Rocket::Core::URL(URLStr));

    mCurrentDoc = mGUIContext->LoadDocument(DocStream);
    if(!mCurrentDoc)
    {
        DocStream->RemoveReference();
        Rocket::Core::Log::Message(Rocket::Core::Log::LT_ERROR,
                                   "Couldn't load document!");
        return false;
    }
    else
        mCurrentDoc->Show();

    mGUIContext->Update();
    update();

    DocStream->RemoveReference();
    return true;
}

void CRenderWidget::initializeGL()
{
    // Set up the GL state.
    glClearColor(0, 0, 0, 1);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width(), height(), 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void CRenderWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1); // set origin to bottom left corner
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    mGUIContext->SetDimensions(Rocket::Core::Vector2i(w,h));

    glClear(GL_COLOR_BUFFER_BIT);
    mGUIContext->Update();
    mGUIContext->Render();
}

void CRenderWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    mGUIContext->Update();
    mGUIContext->Render();
}

void CRenderWidget::mousePressEvent(QMouseEvent* evt)
{
    setFocus();
    mGUIContext->ProcessMouseButtonDown((int)evt->button()-1, GetKeyModifierState(evt->modifiers()));
}

void CRenderWidget::mouseReleaseEvent(QMouseEvent* evt)
{
    mGUIContext->ProcessMouseButtonUp((int)evt->button()-1, GetKeyModifierState(evt->modifiers()));
}

void CRenderWidget::mouseMoveEvent(QMouseEvent* evt)
{
    mGUIContext->ProcessMouseMove(evt->x(), evt->y(), GetKeyModifierState(evt->modifiers()));
}

void CRenderWidget::keyPressEvent(QKeyEvent* evt)
{
    mGUIContext->ProcessKeyDown(mKeyIdentifierMap[(Qt::Key)evt->key()], GetKeyModifierState(evt->modifiers()));
    if(evt->text().data()->toAscii() >= 32)
    {
        mGUIContext->ProcessTextInput(Rocket::Core::String(evt->text().toStdString().c_str()));
    }
    else if(evt->key() == (int)Qt::Key_Return)
        mGUIContext->ProcessTextInput((Rocket::Core::word)'\n');
}

void CRenderWidget::keyReleaseEvent(QKeyEvent* evt)
{
    mGUIContext->ProcessKeyUp(mKeyIdentifierMap[(Qt::Key)evt->key()], GetKeyModifierState(evt->modifiers()));
}

void CRenderWidget::BuildKeyMap()
{
    mKeyIdentifierMap[Qt::Key_unknown] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyIdentifierMap[Qt::Key_Escape] = Rocket::Core::Input::KI_ESCAPE;
    mKeyIdentifierMap[Qt::Key_1] = Rocket::Core::Input::KI_1;
    mKeyIdentifierMap[Qt::Key_2] = Rocket::Core::Input::KI_2;
    mKeyIdentifierMap[Qt::Key_3] = Rocket::Core::Input::KI_3;
    mKeyIdentifierMap[Qt::Key_4] = Rocket::Core::Input::KI_4;
    mKeyIdentifierMap[Qt::Key_5] = Rocket::Core::Input::KI_5;
    mKeyIdentifierMap[Qt::Key_6] = Rocket::Core::Input::KI_6;
    mKeyIdentifierMap[Qt::Key_7] = Rocket::Core::Input::KI_7;
    mKeyIdentifierMap[Qt::Key_8] = Rocket::Core::Input::KI_8;
    mKeyIdentifierMap[Qt::Key_9] = Rocket::Core::Input::KI_9;
    mKeyIdentifierMap[Qt::Key_0] = Rocket::Core::Input::KI_0;
    mKeyIdentifierMap[Qt::Key_Minus] = Rocket::Core::Input::KI_OEM_MINUS;
    mKeyIdentifierMap[Qt::Key_Equal] = Rocket::Core::Input::KI_OEM_PLUS;
    mKeyIdentifierMap[Qt::Key_Back] = Rocket::Core::Input::KI_BACK;
    mKeyIdentifierMap[Qt::Key_Tab] = Rocket::Core::Input::KI_TAB;
    mKeyIdentifierMap[Qt::Key_Q] = Rocket::Core::Input::KI_Q;
    mKeyIdentifierMap[Qt::Key_W] = Rocket::Core::Input::KI_W;
    mKeyIdentifierMap[Qt::Key_E] = Rocket::Core::Input::KI_E;
    mKeyIdentifierMap[Qt::Key_R] = Rocket::Core::Input::KI_R;
    mKeyIdentifierMap[Qt::Key_T] = Rocket::Core::Input::KI_T;
    mKeyIdentifierMap[Qt::Key_Y] = Rocket::Core::Input::KI_Y;
    mKeyIdentifierMap[Qt::Key_U] = Rocket::Core::Input::KI_U;
    mKeyIdentifierMap[Qt::Key_I] = Rocket::Core::Input::KI_I;
    mKeyIdentifierMap[Qt::Key_O] = Rocket::Core::Input::KI_O;
    mKeyIdentifierMap[Qt::Key_P] = Rocket::Core::Input::KI_P;
    mKeyIdentifierMap[Qt::Key_BracketLeft] = Rocket::Core::Input::KI_OEM_4;
    mKeyIdentifierMap[Qt::Key_BracketRight] = Rocket::Core::Input::KI_OEM_6;
    mKeyIdentifierMap[Qt::Key_Return] = Rocket::Core::Input::KI_RETURN;
    mKeyIdentifierMap[Qt::Key_Control] = Rocket::Core::Input::KI_LCONTROL;
    mKeyIdentifierMap[Qt::Key_A] = Rocket::Core::Input::KI_A;
    mKeyIdentifierMap[Qt::Key_S] = Rocket::Core::Input::KI_S;
    mKeyIdentifierMap[Qt::Key_D] = Rocket::Core::Input::KI_D;
    mKeyIdentifierMap[Qt::Key_F] = Rocket::Core::Input::KI_F;
    mKeyIdentifierMap[Qt::Key_G] = Rocket::Core::Input::KI_G;
    mKeyIdentifierMap[Qt::Key_H] = Rocket::Core::Input::KI_H;
    mKeyIdentifierMap[Qt::Key_J] = Rocket::Core::Input::KI_J;
    mKeyIdentifierMap[Qt::Key_K] = Rocket::Core::Input::KI_K;
    mKeyIdentifierMap[Qt::Key_L] = Rocket::Core::Input::KI_L;
    mKeyIdentifierMap[Qt::Key_Semicolon] = Rocket::Core::Input::KI_OEM_1;
    mKeyIdentifierMap[Qt::Key_Apostrophe] = Rocket::Core::Input::KI_OEM_7;
    mKeyIdentifierMap[Qt::Key_Iacute] = Rocket::Core::Input::KI_OEM_3;
    mKeyIdentifierMap[Qt::Key_Shift] = Rocket::Core::Input::KI_LSHIFT;
    mKeyIdentifierMap[Qt::Key_Backslash] = Rocket::Core::Input::KI_OEM_5;
    mKeyIdentifierMap[Qt::Key_Z] = Rocket::Core::Input::KI_Z;
    mKeyIdentifierMap[Qt::Key_X] = Rocket::Core::Input::KI_X;
    mKeyIdentifierMap[Qt::Key_C] = Rocket::Core::Input::KI_C;
    mKeyIdentifierMap[Qt::Key_V] = Rocket::Core::Input::KI_V;
    mKeyIdentifierMap[Qt::Key_B] = Rocket::Core::Input::KI_B;
    mKeyIdentifierMap[Qt::Key_N] = Rocket::Core::Input::KI_N;
    mKeyIdentifierMap[Qt::Key_M] = Rocket::Core::Input::KI_M;
    mKeyIdentifierMap[Qt::Key_Comma] = Rocket::Core::Input::KI_OEM_COMMA;
    mKeyIdentifierMap[Qt::Key_Period] = Rocket::Core::Input::KI_OEM_PERIOD;
    mKeyIdentifierMap[Qt::Key_Slash] = Rocket::Core::Input::KI_OEM_2;
    mKeyIdentifierMap[Qt::Key_Shift] = Rocket::Core::Input::KI_RSHIFT;
    mKeyIdentifierMap[Qt::Key_multiply] = Rocket::Core::Input::KI_MULTIPLY;
    mKeyIdentifierMap[Qt::Key_Menu] = Rocket::Core::Input::KI_LMENU;
    mKeyIdentifierMap[Qt::Key_Space] = Rocket::Core::Input::KI_SPACE;
    mKeyIdentifierMap[Qt::Key_CapsLock] = Rocket::Core::Input::KI_CAPITAL;
    mKeyIdentifierMap[Qt::Key_F1] = Rocket::Core::Input::KI_F1;
    mKeyIdentifierMap[Qt::Key_F2] = Rocket::Core::Input::KI_F2;
    mKeyIdentifierMap[Qt::Key_F3] = Rocket::Core::Input::KI_F3;
    mKeyIdentifierMap[Qt::Key_F4] = Rocket::Core::Input::KI_F4;
    mKeyIdentifierMap[Qt::Key_F5] = Rocket::Core::Input::KI_F5;
    mKeyIdentifierMap[Qt::Key_F6] = Rocket::Core::Input::KI_F6;
    mKeyIdentifierMap[Qt::Key_F7] = Rocket::Core::Input::KI_F7;
    mKeyIdentifierMap[Qt::Key_F8] = Rocket::Core::Input::KI_F8;
    mKeyIdentifierMap[Qt::Key_F9] = Rocket::Core::Input::KI_F9;
    mKeyIdentifierMap[Qt::Key_F10] = Rocket::Core::Input::KI_F10;
    mKeyIdentifierMap[Qt::Key_NumLock] = Rocket::Core::Input::KI_NUMLOCK;
    mKeyIdentifierMap[Qt::Key_ScrollLock] = Rocket::Core::Input::KI_SCROLL;
    mKeyIdentifierMap[Qt::Key_7] = Rocket::Core::Input::KI_7;
    mKeyIdentifierMap[Qt::Key_8] = Rocket::Core::Input::KI_8;
    mKeyIdentifierMap[Qt::Key_9] = Rocket::Core::Input::KI_9;
    mKeyIdentifierMap[Qt::Key_Minus] = Rocket::Core::Input::KI_SUBTRACT;
    mKeyIdentifierMap[Qt::Key_4] = Rocket::Core::Input::KI_4;
    mKeyIdentifierMap[Qt::Key_5] = Rocket::Core::Input::KI_5;
    mKeyIdentifierMap[Qt::Key_6] = Rocket::Core::Input::KI_6;
    mKeyIdentifierMap[Qt::Key_Plus] = Rocket::Core::Input::KI_ADD;
    mKeyIdentifierMap[Qt::Key_1] = Rocket::Core::Input::KI_1;
    mKeyIdentifierMap[Qt::Key_2] = Rocket::Core::Input::KI_2;
    mKeyIdentifierMap[Qt::Key_3] = Rocket::Core::Input::KI_3;
    mKeyIdentifierMap[Qt::Key_0] = Rocket::Core::Input::KI_0;
    mKeyIdentifierMap[Qt::Key_Comma] = Rocket::Core::Input::KI_DECIMAL;
    mKeyIdentifierMap[Qt::Key_Less] = Rocket::Core::Input::KI_OEM_102;
    mKeyIdentifierMap[Qt::Key_F11] = Rocket::Core::Input::KI_F11;
    mKeyIdentifierMap[Qt::Key_F12] = Rocket::Core::Input::KI_F12;
    mKeyIdentifierMap[Qt::Key_F13] = Rocket::Core::Input::KI_F13;
    mKeyIdentifierMap[Qt::Key_F14] = Rocket::Core::Input::KI_F14;
    mKeyIdentifierMap[Qt::Key_F15] = Rocket::Core::Input::KI_F15;
    mKeyIdentifierMap[Qt::Key_Kana_Lock] = Rocket::Core::Input::KI_KANA;
    mKeyIdentifierMap[Qt::Key_yen] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyIdentifierMap[Qt::Key_Equal] = Rocket::Core::Input::KI_OEM_NEC_EQUAL;
    mKeyIdentifierMap[Qt::Key_MediaPrevious] = Rocket::Core::Input::KI_MEDIA_PREV_TRACK;
    mKeyIdentifierMap[Qt::Key_Colon] = Rocket::Core::Input::KI_OEM_1;
    mKeyIdentifierMap[Qt::Key_Minus] = Rocket::Core::Input::KI_OEM_MINUS;
    mKeyIdentifierMap[Qt::Key_Kanji] = Rocket::Core::Input::KI_KANJI;
    mKeyIdentifierMap[Qt::Key_MediaNext] = Rocket::Core::Input::KI_MEDIA_NEXT_TRACK;
    mKeyIdentifierMap[Qt::Key_Enter] = Rocket::Core::Input::KI_NUMPADENTER;
    mKeyIdentifierMap[Qt::Key_Control] = Rocket::Core::Input::KI_RCONTROL;
    mKeyIdentifierMap[Qt::Key_VolumeMute] = Rocket::Core::Input::KI_VOLUME_MUTE;
    mKeyIdentifierMap[Qt::Key_Calculator] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyIdentifierMap[Qt::Key_MediaTogglePlayPause] = Rocket::Core::Input::KI_MEDIA_PLAY_PAUSE;
    mKeyIdentifierMap[Qt::Key_MediaStop] = Rocket::Core::Input::KI_MEDIA_STOP;
    mKeyIdentifierMap[Qt::Key_VolumeDown] = Rocket::Core::Input::KI_VOLUME_DOWN;
    mKeyIdentifierMap[Qt::Key_VolumeUp] = Rocket::Core::Input::KI_VOLUME_UP;
    mKeyIdentifierMap[Qt::Key_HomePage] = Rocket::Core::Input::KI_BROWSER_HOME;
    mKeyIdentifierMap[Qt::Key_Comma] = Rocket::Core::Input::KI_SEPARATOR;
    mKeyIdentifierMap[Qt::Key_division] = Rocket::Core::Input::KI_DIVIDE;
    mKeyIdentifierMap[Qt::Key_Print] = Rocket::Core::Input::KI_SNAPSHOT;
    mKeyIdentifierMap[Qt::Key_Menu] = Rocket::Core::Input::KI_RMENU;
    mKeyIdentifierMap[Qt::Key_Pause] = Rocket::Core::Input::KI_PAUSE;
    mKeyIdentifierMap[Qt::Key_Home] = Rocket::Core::Input::KI_HOME;
    mKeyIdentifierMap[Qt::Key_Up] = Rocket::Core::Input::KI_UP;
    mKeyIdentifierMap[Qt::Key_PageUp] = Rocket::Core::Input::KI_PRIOR;
    mKeyIdentifierMap[Qt::Key_Left] = Rocket::Core::Input::KI_LEFT;
    mKeyIdentifierMap[Qt::Key_Right] = Rocket::Core::Input::KI_RIGHT;
    mKeyIdentifierMap[Qt::Key_End] = Rocket::Core::Input::KI_END;
    mKeyIdentifierMap[Qt::Key_Down] = Rocket::Core::Input::KI_DOWN;
    mKeyIdentifierMap[Qt::Key_PageDown] = Rocket::Core::Input::KI_NEXT;
    mKeyIdentifierMap[Qt::Key_Insert] = Rocket::Core::Input::KI_INSERT;
    mKeyIdentifierMap[Qt::Key_Delete] = Rocket::Core::Input::KI_DELETE;
    mKeyIdentifierMap[Qt::Key_Super_L] = Rocket::Core::Input::KI_LWIN;
    mKeyIdentifierMap[Qt::Key_Super_R] = Rocket::Core::Input::KI_RWIN;
    mKeyIdentifierMap[Qt::Key_ApplicationLeft] = Rocket::Core::Input::KI_APPS;
    mKeyIdentifierMap[Qt::Key_PowerOff] = Rocket::Core::Input::KI_POWER;
    mKeyIdentifierMap[Qt::Key_Sleep] = Rocket::Core::Input::KI_SLEEP;
    mKeyIdentifierMap[Qt::Key_WakeUp] = Rocket::Core::Input::KI_WAKE;
}
