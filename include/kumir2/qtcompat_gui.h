#ifndef KUMIR2_QTCOMPAT_GUI_H
#define KUMIR2_QTCOMPAT_GUI_H

// QtGui-dependent helpers for code shared between Qt4, Qt5 and Qt6 builds.
// Qt4/Qt5 branches keep exactly the original behaviour.

#include <kumir2/qtcompat.h>
#include <QFont>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QWheelEvent>
#include <QString>
#include <cstdlib>

#if QT_VERSION >= 0x060000
#include <QRandomGenerator>
#endif

namespace Kumir2Qt
{

// QFontMetrics::width(), removed in Qt6 (same as horizontalAdvance)
template <typename T>
inline int textWidth(const QFontMetrics &fm, const T &text)
{
#if QT_VERSION >= 0x060000
	return fm.horizontalAdvance(text);
#else
	return fm.width(text);
#endif
}

template <typename T>
inline qreal textWidth(const QFontMetricsF &fm, const T &text)
{
#if QT_VERSION >= 0x060000
	return fm.horizontalAdvance(text);
#else
	return fm.width(text);
#endif
}

// QWheelEvent::delta() and orientation(), removed in Qt6.
// Qt5 derived both from angleDelta(): the dominant axis wins, vertical on tie.
inline Qt::Orientation wheelOrientation(const QWheelEvent *event)
{
#if QT_VERSION >= 0x060000
	const QPoint a = event->angleDelta();
	return qAbs(a.x()) > qAbs(a.y()) ? Qt::Horizontal : Qt::Vertical;
#else
	return event->orientation();
#endif
}

inline int wheelDelta(const QWheelEvent *event)
{
#if QT_VERSION >= 0x060000
	const QPoint a = event->angleDelta();
	return wheelOrientation(event) == Qt::Horizontal ? a.x() : a.y();
#else
	return event->delta();
#endif
}

// qrand(), removed in Qt6: non-negative value in [0, RAND_MAX]
inline int rand()
{
#if QT_VERSION >= 0x060000
	return int(QRandomGenerator::global()->bounded(quint32(RAND_MAX) + 1u));
#else
	return qrand();
#endif
}

// QFont::setWeight() with a Qt4/Qt5 weight value (0..99 scale).
// Qt6 uses OpenType weights (100..900); intermediate values are
// linearly interpolated between the Qt5 named weights.
inline void setLegacyFontWeight(QFont &font, int weight)
{
#if QT_VERSION >= 0x060000
	static const int legacy[] = { 0, 12, 25, 50, 57, 63, 75, 81, 87, 99 };
	static const int openType[] = { 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 };
	weight = qBound(0, weight, 99);
	int result = 900;
	for (int i = 0; i < 9; ++i) {
		if (weight >= legacy[i] && weight <= legacy[i + 1]) {
			const int span = legacy[i + 1] - legacy[i];
			result = openType[i] + (weight - legacy[i]) * (openType[i + 1] - openType[i]) / span;
			break;
		}
	}
	font.setWeight(QFont::Weight(qBound(1, result, 1000)));
#else
	font.setWeight(weight);
#endif
}

} // namespace Kumir2Qt

#endif // KUMIR2_QTCOMPAT_GUI_H
