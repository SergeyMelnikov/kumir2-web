#ifndef ANALIZER_SOURCEFILEINTERFACE_H
#define ANALIZER_SOURCEFILEINTERFACE_H

#include <QtPlugin>
#include <QSet>
#include <QUrl>
#include <QTextStream>
#include <kumir2/qtcompat.h>

namespace Shared
{

namespace Analizer
{

struct SFData {
	QString visibleText;
	QSet<int> protectedLineNumbers;
	QString hiddenText;
	bool hasHiddenText;
	QString sourceEncoding;
	QByteArray hiddenTextSignature;
	QString canonicalSourceLanguageName; // the same as file suffix after '.'
	QUrl sourceUrl;
};

class SourceFileInterface
{
public:
	typedef Shared::Analizer::SFData Data;

public /*methods*/:

	// Abstract methods to read and write from/to string in language-specific way

	virtual QString toString(const SFData &data) const = 0;
	virtual SFData fromString(const QString &string) const = 0;

	// Generic methods to read and write from/to raw byte array. Default implementation
	// assumes UTF-8 encoding with BOM mark, but it is possible to override

	virtual QByteArray toBytes(
		const SFData &data,
		const QString &forceEncoding = ""
	) const {
		if (forceEncoding.length() > 0) {
			return Kumir2Qt::encodeText(toString(data), forceEncoding.toLatin1().constData(), false);
		} else {
			return Kumir2Qt::encodeText(toString(data), "UTF-8", true);
		}
	}

	virtual SFData fromBytes(
		const QByteArray &bytes,
		const QString &forceEncoding = ""
	) const {
		// QTextStream auto-detects Unicode BOM by default, so it is enabled in both cases
		const QByteArray codecName = forceEncoding.length() > 0
			? forceEncoding.toLatin1() : QByteArray("UTF-8");
		QString string = Kumir2Qt::decodeText(bytes, codecName.constData(), true);
		return fromString(string);
	}

};

} // namespace Analizer

} // namespace Shared

Q_DECLARE_INTERFACE(
	Shared::Analizer::SourceFileInterface,
	"kumir2.Analizer.SourceFileInterface"
)

#endif // ANALIZER_SOURCEFILEINTERFACE_H
