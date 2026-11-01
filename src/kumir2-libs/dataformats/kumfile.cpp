#include "kumfile.h"
#include <kumir2/analizer_sourcefileinterface.h>
#include <QRegExp>
#include <QStringList>
#include <QDebug>

//#if !defined(HAS_QCA) && defined(Q_OS_LINUX)
//#   define HAS_QCA
//#endif

#ifdef HAS_QCA
#   include <QtCrypto/qca.h>
#endif


Shared::Analizer::SFData KumFile::insertTeacherMark(
	Shared::Analizer::SFData &data
) { //Inserts |#%% if needed.

	static const QRegExp teacherMark("(^|\\n)\\|#%%");
	if (QRegExp(teacherMark).indexIn(data.visibleText) > -1) {
		//qDebug()<<" TM POS:"<<data.visibleText.indexOf("\n|#%%");
		return data;
	};
	if (QRegExp(teacherMark).indexIn(data.hiddenText) > -1) {
		// qDebug()<<" TM POS:"<<data.visibleText.indexOf("\n|#%%");
		return data;
	};
	// static const QRegExp speclAlg(QString::fromUtf8("^\\s*алг\\s*(\\S\\S\\S)?\\s+@"));
	static const QRegExp speclAlg(QString::fromUtf8("^|\\s*алг\\s*(\\S\\S\\S)?\\s*@"));

	int pos = QRegExp(speclAlg).indexIn(data.hiddenText);
	qDebug() << data.hiddenText;
	if (pos > -1) {

		data.hiddenText.insert(pos, "\n|#%%\n");
		return data;
	};
	pos = QRegExp(speclAlg).indexIn(data.visibleText);
	if (pos > 1) {
		data.visibleText.insert(pos, "\n|#%%\n");
		QStringList strings = data.visibleText.split("\n") /* KeepEmptyParts is default */;
		for (int i = 0; i < strings.count(); i++) {
			if (strings.at(i).startsWith("|#%%")) {
				data.protectedLineNumbers.insert(i);
				break;
			}
		}
		return data;
	}
	return data;
}


bool KumFile::hasCryptographicRoutines()
{
#ifdef HAS_QCA
	return true;
#else
	return false;
#endif
}

void KumFile::generateKeyPair(
	const QString &passPhrase,
	QString &privateKey,
	QString &publicKey
) {
	privateKey.clear();
	publicKey.clear();
#ifdef HAS_QCA
	QCA::PrivateKey seckey = QCA::KeyGenerator().createRSA(1024);
	QCA::SecureArray phrase(passPhrase.toUtf8());
	privateKey = seckey.toPEM(phrase);
	QCA::PublicKey pubkey = seckey.toPublicKey();
	publicKey = pubkey.toPEM();
#endif
}

void KumFile::signHiddenText(
	Shared::Analizer::SFData &data,
	const QString &privateKey,
	const QString &passPhrase
) {
#ifdef HAS_QCA
	QCA::SecureArray phrase(passPhrase.toUtf8());
	QCA::PrivateKey seckey = QCA::PrivateKey::fromPEM(privateKey, phrase);
	seckey.startSign(QCA::EMSA3_MD5);
	seckey.update(data.hiddenText.trimmed().toUtf8());
	data.hiddenTextSignature = seckey.signature();
#else
	data.hiddenTextSignature.clear();
#endif
}

KumFile::VerifyResult KumFile::verifyHiddenText(
	const Shared::Analizer::SFData &data,
	const QString &publicKey
) {
#ifdef HAS_QCA
	if (data.hiddenTextSignature.isEmpty()) {
		return NoSignature;
	}
	QCA::PublicKey pubkey = QCA::PublicKey::fromPEM(publicKey);
	pubkey.startVerify(QCA::EMSA3_MD5);
	pubkey.update(data.hiddenText.trimmed().toUtf8());
	return pubkey.validSignature(data.hiddenTextSignature)
		? SignatureMatch : SignatureMismatch;
#else
	return CryptographyNotSupported;
#endif
}
