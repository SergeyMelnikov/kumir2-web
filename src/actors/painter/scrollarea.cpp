#include "scrollarea.h"
#include <kumir2/qtcompat_gui.h>

#include <QWheelEvent>
#include <QScrollBar>

namespace ActorPainter {

ScrollArea::ScrollArea(QWidget *parent) :
    QScrollArea(parent)
{
}

void ScrollArea::wheelEvent(QWheelEvent *evt)
{
    const int delta = Kumir2Qt::wheelDelta(evt);
    const int degrees = delta / 8;
    const int steps = -1 * degrees / 15;
    if (! (evt->modifiers() & Qt::ControlModifier) ) {
        // Scroll contents
        QAbstractSlider* slider = Kumir2Qt::wheelOrientation(evt) == Qt::Vertical
                ? verticalScrollBar()
                : horizontalScrollBar();
        if (Kumir2Qt::wheelOrientation(evt) == Qt::Vertical && evt->modifiers() & Qt::ShiftModifier) {
            slider = horizontalScrollBar();
        }
        if (slider->isVisible()) {
            int value = slider->value() + steps * slider->singleStep();
            value = qMin(value, slider->maximum());
            slider->setValue(value);
            evt->accept();
        }
        else {
            evt->ignore();
        }
    }
    else {
        // Zoom
        emit changeZoomRequest(steps);
        evt->accept();
    }
}

} // namespace ActorPainter
