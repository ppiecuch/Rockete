/**********************************************************************************
 * License notice
 * ------------------------
 * qtsysteminterface.cpp
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

#include "qtsysteminterface.hpp"

CQtSystemInterface::CQtSystemInterface(QTextEdit* LogWidget)
    : mLogWidget(LogWidget),
      mLogDoc(new QTextDocument(LogWidget))
{
    mTimer.start();
    mLogWidget->setDocument(mLogDoc);

    mWarnFormat.setForeground(QBrush(QColor::fromRgb(242,170,46)));
    mWarnFormat.setFontItalic(true);
    mErrFormat.setForeground(Qt::red);
    mErrFormat.setFontWeight(QFont::Bold);
    mInfoFormat.setForeground(Qt::black);
}

float CQtSystemInterface::GetElapsedTime()
{
    return static_cast<float>(mTimer.elapsed());
}

bool CQtSystemInterface::LogMessage(Rocket::Core::Log::Type type,
                                    const Rocket::Core::String &message)
{
    QTextCursor Cursor = mLogWidget->textCursor();
    QString LogMessage;
    switch(type)
    {
    case Rocket::Core::Log::LT_ERROR:
        LogMessage = "ERROR:" + QString(message.CString());
        LogMessage += QString("\n");
        Cursor.insertText(LogMessage, mErrFormat);
        break;
    case Rocket::Core::Log::LT_WARNING:
        LogMessage = "WARNING:" + QString(message.CString());
        LogMessage += QString("\n");
        Cursor.insertText(LogMessage, mWarnFormat);
        break;
    default: LogMessage = "INFO:" + QString(message.CString());
        LogMessage += QString("\n");
        Cursor.insertText(LogMessage, mInfoFormat);
        break;
    }

    mLogWidget->setTextCursor(Cursor);

    return true;
}
