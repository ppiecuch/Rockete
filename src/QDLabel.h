#ifndef QDLABEL_H
#define QDLABEL_H

#include <QLabel>

// Simple label extension with
// resize event.

class QDLabel : public QLabel
{
    Q_OBJECT
public:
    QDLabel(QWidget *parent = 0, Qt::WindowFlags flags = 0) : QLabel(parent, flags) { }
    void resizeEvent(QResizeEvent * event) { resizeLabel(event); }
signals:
    void resizeLabel(QResizeEvent * event);
};

#endif // QDLABEL_H
