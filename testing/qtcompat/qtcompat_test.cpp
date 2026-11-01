// Prints the results of Qt-version dependent operations used by the Qt6 port.
// The output must be identical when built with Qt5 and with Qt6:
//   Qt5 build: helpers call the original Qt5 API (reference behaviour)
//   Qt6 build: helpers use the Qt6 re-implementations
// See testing/qtcompat/run.sh

#include <kumir2/qtcompat.h>

#include <QCoreApplication>
#include <QMap>
#include <QStringList>
#include <QTextCodec>
#include <cstdio>

static void out(const char *name, const QByteArray &value)
{
	std::printf("%s: %s\n", name, value.constData());
}

static QByteArray hex(const QByteArray &b) { return b.toHex(); }

// Qt-version independent checksum (qHash differs between Qt5 and Qt6)
static QByteArray fnv1a(const QByteArray &b)
{
	quint32 h = 2166136261u;
	for (int i = 0; i < b.size(); ++i) {
		h ^= quint8(b.at(i));
		h *= 16777619u;
	}
	return QByteArray::number(h);
}
static QByteArray hex(const QString &s)
{
	QByteArray r;
	for (int i = 0; i < s.size(); ++i) {
		r += QByteArray::number(s.at(i).unicode(), 16) + ' ';
	}
	return r;
}

struct Item {
	int key;
	int id;
	bool operator<(const Item &other) const { return key < other.key; }
};

static quint32 lcgState = 12345;
static int lcg(int mod)
{
	lcgState = lcgState * 1103515245u + 12345u;
	return int((lcgState >> 16) % quint32(mod));
}

static void testSort()
{
	// Unstable sort: order of items with equal keys must be the same as Qt5 qSort
	QByteArray all;
	for (int round = 0; round < 300; ++round) {
		QList<Item> list;
		const int n = lcg(60);
		const int range = 1 + lcg(8);
		for (int i = 0; i < n; ++i) {
			Item it = { lcg(range), i };
			list.append(it);
		}
		Kumir2Qt::qtSort(list);
		for (int i = 0; i < list.size(); ++i) {
			all += QByteArray::number(list[i].id) + ',';
		}
		all += ';';
	}
	out("qtSort.length", QByteArray::number(all.size()));
	out("qtSort.fnv1a", fnv1a(all));
}

static void testMultiMap()
{
#if QT_VERSION >= 0x060000
	QMultiMap<QString, QString> fields;
	fields.insert("b", "1");
	fields.insert("a", "2");
	fields.insert("b", "3");
	fields.insert("b", "4");
#else
	QMap<QString, QString> fields;
	fields.insertMulti("b", "1");
	fields.insertMulti("a", "2");
	fields.insertMulti("b", "3");
	fields.insertMulti("b", "4");
#endif
	out("multimap.values(b)", fields.values("b").join(",").toUtf8());
	out("multimap.keys", fields.keys().join(",").toUtf8());
	out("multimap.count", QByteArray::number(fields.count()));
}

static void testCodecs()
{
	const QString text = QString::fromUtf8("алг Привет, мир! ёЁ №");
	const QByteArray cp1251 = QTextCodec::codecForName("CP1251")->fromUnicode(text);
	const QByteArray utf8 = text.toUtf8();
	const QByteArray utf8bom = QByteArray("\xEF\xBB\xBF") + utf8;
	QByteArray utf16le;
	for (int i = 0; i < text.size(); ++i) {
		utf16le += char(text.at(i).unicode() & 0xFF);
		utf16le += char(text.at(i).unicode() >> 8);
	}
	const QByteArray utf16lebom = QByteArray("\xFF\xFE", 2) + utf16le;

	out("decode.utf8", hex(Kumir2Qt::decodeText(utf8, "UTF-8")));
	out("decode.utf8bom", hex(Kumir2Qt::decodeText(utf8bom, "UTF-8")));
	out("decode.cp1251", hex(Kumir2Qt::decodeText(cp1251, "CP1251")));
	out("decode.cp1251+utf8bom", hex(Kumir2Qt::decodeText(utf8bom, "CP1251")));
	out("decode.cp1251+utf8bom.noauto", hex(Kumir2Qt::decodeText(utf8bom, "CP1251", false)));
	out("decode.utf16.le-bom", hex(Kumir2Qt::decodeText(utf16lebom, "UTF-16")));
	out("decode.utf16le", hex(Kumir2Qt::decodeText(utf16le, "UTF-16LE")));
	out("decode.utf16+utf8bom", hex(Kumir2Qt::decodeText(utf8bom, "UTF-16")));
	out("decode.cp866", hex(Kumir2Qt::decodeText(QTextCodec::codecForName("CP866")->fromUnicode(text), "CP866")));
	out("decode.koi8r", hex(Kumir2Qt::decodeText(QTextCodec::codecForName("KOI8-R")->fromUnicode(text), "KOI8-R")));

	out("encode.utf8", hex(Kumir2Qt::encodeText(text, "UTF-8")));
	out("encode.utf8bom", hex(Kumir2Qt::encodeText(text, "UTF-8", true)));
	out("encode.cp1251", hex(Kumir2Qt::encodeText(text, "CP1251")));
	out("encode.cp1251.bom", hex(Kumir2Qt::encodeText(text, "CP1251", true)));
	out("encode.utf16le", hex(Kumir2Qt::encodeText(text, "UTF-16LE")));
	out("encode.utf16le.bom", hex(Kumir2Qt::encodeText(text, "UTF-16LE", true)));

	out("split.skip", QString("a;;b;").split(";", Kumir2Qt::SkipEmptyParts).join("|").toUtf8());
	out("split.keep", QString("a;;b;").split(";", Kumir2Qt::KeepEmptyParts).join("|").toUtf8());

	QSet<QString> set;
	set << "x";
	out("setToList", Kumir2Qt::setToList(set).join(",").toUtf8());
}

static void testStringQuirks()
{
	// lexer.cpp: "QString error = 0; ... if (error > 0)"
	const QString samples[] = { QString(), QString(""), QString("x") };
	QByteArray cmp;
	for (const QString &s : samples) {
#if QT_VERSION >= 0x060000
		cmp += (!s.isEmpty()) ? '1' : '0';
#else
		cmp += (s > 0) ? '1' : '0';
#endif
	}
	out("quirk.string>0", cmp);

	// robotmodule.cpp: "QString + int" and syntaxanalizer.cpp: "QString = int"
	const int codes[] = { 0, 3, 65, 200, 255, 300, -5 };
	QByteArray plus, assign;
	for (int err : codes) {
#if QT_VERSION >= 0x060000
		const QString p = QString("code=") + QChar::fromLatin1(char(err));
		QString a;
		a = QString(QChar::fromLatin1(char(err)));
#else
		const QString p = QString("code=") + err;
		QString a;
		a = err;
#endif
		plus += hex(p) + '/';
		assign += hex(a) + '/';
	}
	out("quirk.string+int", plus);
	out("quirk.string=int", assign);
}

int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);
	testSort();
	testMultiMap();
	testCodecs();
	testStringQuirks();
	return 0;
}
