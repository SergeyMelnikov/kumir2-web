#ifndef WIDGETS_FILEDIALOG_H
#define WIDGETS_FILEDIALOG_H

#include <QByteArray>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

class QWidget;

#ifdef WIDGETS_LIBRARY
#define WIDGETS_EXPORT Q_DECL_EXPORT
#else
#define WIDGETS_EXPORT Q_DECL_IMPORT
#endif

namespace Widgets
{

/** Replacement of QFileDialog static functions for files on the user's
 *  computer. Desktop builds call QFileDialog. In the browser (WebAssembly)
 *  the application sees only its virtual file system, so these functions
 *  use the browser's file pickers and copy files to and from it. */
class WIDGETS_EXPORT FileDialog
{
	Q_DECLARE_TR_FUNCTIONS(Widgets::FileDialog)
public:
	/** Same as QFileDialog::getOpenFileName. In the browser the chosen file
	 *  is copied to a new directory of the virtual file system; its path
	 *  there is returned. Empty string means cancel. */
	static QString getOpenFileName(
		QWidget *parent,
		const QString &caption = QString(),
		const QString &dir = QString(),
		const QString &filter = QString()
	);

	/** Same as QFileDialog::getOpenFileNames. In the browser all chosen
	 *  files are copied to one new directory, so relative references
	 *  between them (a course and its tasks) keep working. */
	static QStringList getOpenFileNames(
		QWidget *parent,
		const QString &caption = QString(),
		const QString &dir = QString(),
		const QString &filter = QString()
	);

	/** Same as QFileDialog::getExistingDirectory. In the browser the chosen
	 *  folder with all its files and subfolders is copied to the virtual
	 *  file system; the path of the copy is returned. */
	static QString getExistingDirectory(
		QWidget *parent,
		const QString &caption = QString(),
		const QString &dir = QString()
	);

	/** Same as QFileDialog::getSaveFileName. In the browser the returned
	 *  path is in a new directory of the virtual file system: the caller
	 *  writes the file there as usual, then calls finishSave() which hands
	 *  it to the browser. The caller may change the file name (for example,
	 *  add a suffix) but not the directory. */
	static QString getSaveFileName(
		QWidget *parent,
		const QString &caption = QString(),
		const QString &dir = QString(),
		const QString &filter = QString()
	);

	/** How finishSave delivers a file in the browser */
	enum Delivery {
		/** Explicit save: write the browser file or start a download */
		Deliver,
		/** Automatic save of a file already delivered: write the browser
		 *  file again, but do not start another download */
		Update
	};

	/** Browser: delivers the file written to @param path (returned by
	 *  getSaveFileName) to the user's computer: writes it to the file chosen
	 *  in the browser save picker, or starts a download. Shows an error and
	 *  returns false on failure. Desktop, or a path not returned by
	 *  getSaveFileName: does nothing and returns true. */
	static bool finishSave(QWidget *parent, const QString &path, Delivery delivery = Deliver);

	enum SaveResult {
		Saved,
		Cancelled,
		Failed,
		/** Browser without a save picker: a download was started; the
		 *  browser does not tell whether the user kept the file */
		Downloaded
	};

	/** Saves @param contents to a file chosen by the user (browser: the
	 *  browser save picker, or a download named @param suggestedName).
	 *  Shows no messages. */
	static SaveResult saveFileContent(
		QWidget *parent,
		const QByteArray &contents,
		const QString &suggestedName,
		const QString &filter = QString()
	);
};

} // namespace Widgets

#endif // WIDGETS_FILEDIALOG_H
