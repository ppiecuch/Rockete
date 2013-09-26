#include "ToolTest.h"

#include "Rockete.h"
#include "RocketHelper.h"
#include "OpenedDocument.h"
#include "ActionManager.h"
#include "ActionSetInlineProperty.h"
#include "ActionInsertElement.h"
#include "ActionGroup.h"
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>
#include <QToolButton>

#define MARKER_INDEX_BOTTOM_RIGHT 0
#define MARKER_INDEX_TOP_RIGHT 1

// available frames:
struct TestFrameInfo {
    const char *image;
    const char *label;
    QRect screen_space;
    int w, h;
} testFrames[] = {
    { ":/images/frame-android-medium.png", "Android", QRect(0,0,10,10), 320, 480 },
    { ":/images/frame-ipad.png", "iPad", QRect(0,0,10,10), 320, 480 },
    { ":/images/frame-playbook.png", "Playbook", QRect(0,0,10,10), 320, 480 },
    { ":/images/frame-android-small.png", "Android", QRect(0,0,10,10), 320, 480 },
    { ":/images/frame-iphone.png", "iPhone", QRect(0,0,10,10), 320, 480 },
    { ":/images/frame-z10.png", "BB Z10", QRect(0,0,10,10), 320, 480 },
    { ":/images/frame-android-xsmall.png", "Android", QRect(0,0,10,10), 320, 480 },
    { ":/images/frame-iphone5.png", "iPhone 5", QRect(0,0,10,10), 320, 480 },
    { NULL }
};

const int kGridSize = 3;

ToolTest::ToolTest()
: Tool()
{
    QLayout *layout;

    name = "Test tool";
    imageName = ":/images/tool_test.png";
    QGridLayout *glayout = new QGridLayout();
    glayout->setMargin(2);
    glayout->setSpacing(2);
    glayout->addWidget(new QLabel("<table align=center<tr><td><img src=':/images/tool_test.png'></td><td valign=middle>Tool: <b>Test device resolutions</b></td></tr></table>"), 0, 0, 1, kGridSize, Qt::AlignCenter);
    TestFrameInfo *e = &testFrames[0]; int c = 0, r = 1; while(e->image) {
        QImage result = QImage(e->image).scaled(128, 128).scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QToolButton *b = new QToolButton();
        #ifdef Q_WS_MACX
            b->setAttribute(Qt::WA_MacMiniSize);
        #endif
        // b->setFlat(true);
        b->setContentsMargins(4, 4, 4, 4);
        b->setAutoFillBackground(true);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setText(e->label);
        b->setIcon(QIcon(e->image));
        b->setIconSize(QSize(64, 64));
        b->setToolTip(QString("%1 %2x%3").arg(e->label).arg(e->w).arg(e->h));
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        glayout->addWidget(b,r,c); ++c; if (c == kGridSize) { c = 0; ++r; }
        ++e;
    }
    layout = glayout;
    widget = new QWidget();
    widget->setLayout(layout);
}

void ToolTest::onElementClicked(Element *_element)
{
    if (_element) {

        Element *parent;
        Rocket::Core::String name, value;

        parent = _element;
        bool hasOnClick = false;

        do
        {
            int index = 0;
            while(parent->IterateAttributes<Rocket::Core::String>(index, name, value))
            {
                if(name=="onclick")
                {
                    hasOnClick = true;
                }
            }

            if(!hasOnClick)
            {
                parent = parent->GetParentNode();
            }
        } while (!hasOnClick && parent != NULL);

        return;
    }
}

// Private slots:

// Private:
