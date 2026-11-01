#ifndef KUMIR2_QTCOMPAT_H
#define KUMIR2_QTCOMPAT_H

// Helpers for code shared between Qt4, Qt5 and Qt6 builds.
// Qt4/Qt5 branches keep exactly the original behaviour; Qt6 branches
// reproduce it using Qt6 (and Qt6 Core5Compat) API.

#include <QtGlobal>
#include <QByteArray>
#include <QIODevice>
#include <QString>
#include <QTextStream>
#include <QTextCodec> // Qt6: provided by Core5Compat
#include <QList>
#include <QSet>
#include <QVector>
#include <QtAlgorithms>
#include <QStringList>
#include <list>
#include <vector>

#if QT_VERSION >= 0x060000
#include <QStringConverter>
#include <optional>
#endif

namespace Kumir2Qt
{

// Equivalent of QTextStream::setCodec(codecName) for Unicode encodings
// ("UTF-8", "UTF-16", "UTF-16LE", ...). Qt6 QTextStream can not use legacy
// 8-bit encodings: use decodeText()/encodeText() for those.
inline void setStreamCodec(QTextStream &ts, const char *codecName)
{
#if QT_VERSION >= 0x060000
	std::optional<QStringConverter::Encoding> enc =
		QStringConverter::encodingForName(codecName);
	if (enc) {
		ts.setEncoding(*enc);
	} else {
		qWarning("Kumir2Qt::setStreamCodec: encoding '%s' is not supported by Qt6 QTextStream", codecName);
	}
#else
	ts.setCodec(codecName);
#endif
}

// Decodes bytes the same way as Qt4/Qt5 QTextStream does after
// setCodec(codec) and setAutoDetectUnicode(autoDetectUnicode).
// Null or unknown codec means "keep stream default" (locale codec).
inline QString decodeText(const QByteArray &bytes, QTextCodec *codec, bool autoDetectUnicode = true)
{
#if QT_VERSION >= 0x060000
	std::optional<QStringConverter::Encoding> enc;
	if (codec) {
		enc = QStringConverter::encodingForName(codec->name().constData());
		if (!enc) {
			// Legacy 8-bit codec. The stream would switch to Unicode only if a BOM present
			const bool hasBom = autoDetectUnicode && QStringConverter::encodingForData(bytes).has_value();
			if (!hasBom) {
				return codec->toUnicode(bytes);
			}
			enc = QStringConverter::Utf8; // replaced by the BOM-detected encoding
		}
	} else {
		enc = QStringConverter::System;
	}
	QTextStream ts(bytes);
	ts.setEncoding(*enc);
	ts.setAutoDetectUnicode(autoDetectUnicode);
	return ts.readAll();
#else
	QTextStream ts(bytes);
	if (codec) {
		ts.setCodec(codec);
	}
	ts.setAutoDetectUnicode(autoDetectUnicode);
	return ts.readAll();
#endif
}

inline QString decodeText(const QByteArray &bytes, const char *codecName, bool autoDetectUnicode = true)
{
	return decodeText(bytes, QTextCodec::codecForName(codecName), autoDetectUnicode);
}

// Encodes text the same way as Qt4/Qt5 QTextStream does after
// setCodec(codec) and setGenerateByteOrderMark(generateBOM).
// Null or unknown codec means "keep stream default" (locale codec).
inline QByteArray encodeText(const QString &text, QTextCodec *codec, bool generateBOM = false)
{
	QByteArray buffer;
#if QT_VERSION >= 0x060000
	std::optional<QStringConverter::Encoding> enc;
	if (codec) {
		enc = QStringConverter::encodingForName(codec->name().constData());
		if (!enc) {
			// Legacy 8-bit codec: byte order mark is not applicable
			return codec->fromUnicode(text);
		}
	} else {
		enc = QStringConverter::System;
	}
	QTextStream ts(&buffer, QIODevice::WriteOnly);
	ts.setEncoding(*enc);
	ts.setGenerateByteOrderMark(generateBOM);
	ts << text;
	ts.flush();
#else
	QTextStream ts(&buffer, QIODevice::WriteOnly);
	if (codec) {
		ts.setCodec(codec);
	}
	ts.setGenerateByteOrderMark(generateBOM);
	ts << text;
	ts.flush();
#endif
	return buffer;
}

inline QByteArray encodeText(const QString &text, const char *codecName, bool generateBOM = false)
{
	return encodeText(text, QTextCodec::codecForName(codecName), generateBOM);
}

// QString::split behavior (moved to Qt namespace in Qt6)
#if QT_VERSION >= 0x060000
static const Qt::SplitBehavior SkipEmptyParts = Qt::SkipEmptyParts;
static const Qt::SplitBehavior KeepEmptyParts = Qt::KeepEmptyParts;
#else
static const QString::SplitBehavior SkipEmptyParts = QString::SkipEmptyParts;
static const QString::SplitBehavior KeepEmptyParts = QString::KeepEmptyParts;
#endif

template <typename T>
inline QList<T> setToList(const QSet<T> &set)
{
#if QT_VERSION >= 0x060000
	return QList<T>(set.begin(), set.end());
#else
	return set.toList();
#endif
}

// qSort() replacement. It is not stable, so the order of equal elements
// depends on the algorithm: Qt6 branch is a copy of the Qt4/Qt5 qSort
// implementation (QAlgorithmsPrivate::qSortHelper) to keep exactly the same order.
#if QT_VERSION >= 0x060000
namespace Private
{
template <typename RandomAccessIterator>
void qtSortHelper(RandomAccessIterator start, RandomAccessIterator end)
{
top:
	int span = int(end - start);
	if (span < 2) {
		return;
	}

	--end;
	RandomAccessIterator low = start, high = end - 1;
	RandomAccessIterator pivot = start + span / 2;

	if (*end < *start) {
		qSwap(*end, *start);
	}
	if (span == 2) {
		return;
	}

	if (*pivot < *start) {
		qSwap(*pivot, *start);
	}
	if (*end < *pivot) {
		qSwap(*end, *pivot);
	}
	if (span == 3) {
		return;
	}

	qSwap(*pivot, *end);

	while (low < high) {
		while (low < high && *low < *end) {
			++low;
		}

		while (high > low && *end < *high) {
			--high;
		}

		if (low < high) {
			qSwap(*low, *high);
			++low;
			--high;
		} else {
			break;
		}
	}

	if (*low < *end) {
		++low;
	}

	qSwap(*end, *low);
	qtSortHelper(start, low);

	start = low + 1;
	++end;
	goto top;
}
} // namespace Private
#endif

template <typename Container>
inline void qtSort(Container &c)
{
#if QT_VERSION >= 0x060000
	if (c.size() > 1) {
		Private::qtSortHelper(c.begin(), c.end());
	}
#else
	qSort(c);
#endif
}

// QList/QVector <-> STL conversions (removed from Qt6 containers)

template <typename T>
inline std::list<T> toStdList(const QList<T> &list)
{
#if QT_VERSION >= 0x060000
	return std::list<T>(list.begin(), list.end());
#else
	return list.toStdList();
#endif
}

template <typename T>
inline QList<T> listFromStdList(const std::list<T> &list)
{
#if QT_VERSION >= 0x060000
	return QList<T>(list.begin(), list.end());
#else
	return QList<T>::fromStdList(list);
#endif
}

template <typename T>
inline std::vector<T> toStdVector(const QVector<T> &vector)
{
#if QT_VERSION >= 0x060000
	return std::vector<T>(vector.begin(), vector.end());
#else
	return vector.toStdVector();
#endif
}

} // namespace Kumir2Qt

#endif // KUMIR2_QTCOMPAT_H
