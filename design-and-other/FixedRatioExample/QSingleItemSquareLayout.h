#ifndef QSINGLEITEMSQUARELAYOUT_H
#define QSINGLEITEMSQUARELAYOUT_H

#include <QLayout>
#include <QPointer>
#include <QRect>
#include <QWidgetItem>
#include <QLayoutItem>
#include <QWidget>

/*
 * http://wiki.forum.nokia.com/index.php/CS001309_-_Maintaining_square_form_for_a_widget_in_Qt
 */

class QSingleItemSquareLayout : public QLayout
{
    Q_OBJECT

public:
    QSingleItemSquareLayout(QWidget* parent, int spacing =-1);
    QSingleItemSquareLayout(int spacing = -1);
    ~QSingleItemSquareLayout();

    virtual void add(QLayoutItem* item);


    virtual void addItem(QLayoutItem* item);

    virtual void addWidget(QWidget* widget);

    virtual QLayoutItem* takeAt(int index);

    virtual QLayoutItem* itemAt(int index) const;

    virtual int count() const;

    /*
     * These are ours since we do have only one item.
     */
    virtual QLayoutItem* replaceItem(QLayoutItem* item);
    virtual QLayoutItem* take();
    virtual bool hasItem() const;


    virtual Qt::Orientations expandingDirections() const;

    /*
     * This method contains most of the juice of this article.
     * http://doc.trolltech.com/qlayoutitem.html#setGeometry
     */
    virtual void setGeometry(const QRect& rect);

    virtual QRect geometry();


    virtual QSize sizeHint() const;

    virtual QSize minimumSize() const;

    virtual bool hasHeightForWidth() const;

private:

    /*
     * Saves the last received rect.
     */
    void setLastReceivedRect(const QRect& rect);

    /*
     * Used to initialize the object.
     */
    void init(int spacing);

    /*
     * Calculates the maximum size for the item from the assigned size.
     */
    QSize calculateProperSize(QSize from) const;

    /*
     * Added for golden ratio rectangle.
     */
    QSize calculateGoldenRatioSize(QSize from) const;

    /*
     * Calculates the center location from the assigned size and the items size.
     */
    QPoint calculateCenterLocation(QSize from, QSize itemSize) const;

    /*
     * Added for top left location.
     */
    QPoint calculateTopLeftLocation(QSize from, QSize itemSize) const;

    /*
     * Check if two QRects are equal
     */
    bool areRectsEqual(const QRect& a, const QRect& b) const;

    /*
     * Contains item reference
     */
    QLayoutItem* item;

    /*
     * Used for caching so we won't do calculations every time setGeometry is called.
     */
    QRect* lastReceivedRect;
    /*
     * Contains geometry
     */
    QRect* _geometry;

    QWidget * parent;

};

#endif // QSINGLEITEMSQUARELAYOUT_H
