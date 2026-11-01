#ifndef KUMIRANALIZER_KUMFILEHANDLER_H
#define KUMIRANALIZER_KUMFILEHANDLER_H

#include <QObject>
#include <kumir2/analizer_sourcefileinterface.h>
#include <kumir2/qtcompat.h>

namespace KumirAnalizer {

using namespace Shared::Analizer;

class KumFileHandler
        : public QObject
        , public SourceFileInterface
{
    Q_OBJECT
    Q_INTERFACES(Shared::Analizer::SourceFileInterface)

public:
    explicit KumFileHandler(QObject *parent = 0);

    QString toString(const Data & data) const;
    Data fromString(const QString & string) const;

    // KumFile implementation shoud use UTF-16 encoding if no BOM present
    // to allows compatibility with Kumir 1.x
    inline Data fromBytes(const QByteArray & bytes, const QString & forceEncoding = "") const {
        // QTextStream auto-detects Unicode BOM by default, so it is enabled in both cases
        const QByteArray codecName = forceEncoding.length() > 0
            ? forceEncoding.toLatin1() : QByteArray("UTF-16");
        QString string = Kumir2Qt::decodeText(bytes, codecName.constData(), true);
        return fromString(string);
    }
private:
    static QString& trimLeadingSpaces(QString &line);
};

} // namespace KumirAnalizer

#endif // KUMIRANALIZER_KUMFILEHANDLER_H
