#include "filedialog.h"

#include <QFile>
#include <QFileDialog>

#if defined(Q_OS_WASM) && defined(KUMIR2_WASM_ASYNCIFY)
#define KUMIR2_BROWSER_FILE_DIALOGS
#endif

#ifdef KUMIR2_BROWSER_FILE_DIALOGS
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QHash>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTimer>
#include <emscripten.h>

// Browser side. Each kumir2Start* function starts a browser operation (a
// file picker or a write) and returns at once; the operation keeps its state
// in Module.kumir2Operation. The C++ side waits for it in a nested Qt event
// loop, like a modal dialog does (see waitForBrowser).
//
// The operation is not awaited in an EM_ASYNC_JS function: while C++ code is
// suspended there, Qt queues DOM events and handles them after the user has
// chosen a file. Qt 6.10 fails on queued pointer enter/leave events (a null
// element in dom::mapPoint): Chrome sends them when a closed menu popup is
// removed from the page, so choosing a menu item that opened a picker stopped
// the application.
//
// Pickers are called before the first "await": browsers allow them only
// while handling user input.

// Lets the user choose files to open. @param accept: comma separated
// extensions (".kum,.txt") or empty for any file. @param mode: 0 - one file,
// 1 - several files, 2 - a folder with all its files (names are relative
// paths starting with the folder name). Result: the number of chosen files
// kept in Module.kumir2PickedFiles, 0 on cancel, -1 on failure.
EM_JS(void, kumir2StartPickFiles, (const char *acceptUtf8, int mode), {
    const operation = Module.kumir2Operation = { done: false, result: -1 };
    const accept = UTF8ToString(acceptUtf8);
    const extensions = accept ? accept.split(",") : [];
    const finish = result => {
        operation.result = result;
        operation.done = true;
    };
    const readEntries = async entries => {
        const encoder = new TextEncoder();
        Module.kumir2PickedFiles = await Promise.all(entries.map(async entry => ({
            name: encoder.encode(entry.path),
            data: new Uint8Array(await entry.file.arrayBuffer())
        })));
        return Module.kumir2PickedFiles.length;
    };
    const failed = error => {
        if (error && error.name === "AbortError")
            return finish(0);
        console.warn("Browser file open failed", error);
        finish(-1);
    };
    try {
        const picker = mode === 2 ? window.showDirectoryPicker : window.showOpenFilePicker;
        if (typeof picker === "function") {
            let picking;
            if (mode === 2) {
                picking = window.showDirectoryPicker();
            } else {
                const options = { multiple: mode === 1 };
                if (extensions.length)
                    options.types = [{ accept: { "application/octet-stream": extensions } }];
                picking = window.showOpenFilePicker(options);
            }
            picking.then(async picked => {
                const entries = [];
                if (mode === 2) {
                    const walk = async (directory, prefix) => {
                        for await (const [name, handle] of directory.entries()) {
                            if (handle.kind === "file")
                                entries.push({ path: prefix + name, file: await handle.getFile() });
                            else
                                await walk(handle, prefix + name + "/");
                        }
                    };
                    await walk(picked, picked.name + "/");
                } else {
                    for (const handle of picked) {
                        const file = await handle.getFile();
                        entries.push({ path: file.name, file });
                    }
                }
                return readEntries(entries);
            }).then(finish, failed);
        } else {
            // <input type="file">: the "cancel" event is supported by current browsers
            const input = document.createElement("input");
            input.type = "file";
            input.multiple = mode !== 0;
            input.webkitdirectory = mode === 2;
            if (accept && mode !== 2)
                input.accept = accept;
            input.style.display = "none";
            document.body.appendChild(input);
            const done = files => {
                input.remove();
                readEntries(files.map(file => ({ path: file.webkitRelativePath || file.name, file })))
                    .then(finish, failed);
            };
            input.addEventListener("change", () => done(Array.from(input.files || [])), { once: true });
            input.addEventListener("cancel", () => done([]), { once: true });
            input.click();
        }
    } catch (error) {
        failed(error);
    }
});

EM_JS(int, kumir2PickedNameSize, (int index), {
    return Module.kumir2PickedFiles[index].name.length;
});

EM_JS(void, kumir2PickedNameCopy, (int index, char *out), {
    HEAPU8.set(Module.kumir2PickedFiles[index].name, out);
});

EM_JS(int, kumir2PickedDataSize, (int index), {
    return Module.kumir2PickedFiles[index].data.length;
});

EM_JS(void, kumir2PickedDataCopy, (int index, char *out), {
    HEAPU8.set(Module.kumir2PickedFiles[index].data, out);
});

EM_JS(void, kumir2PickedClear, (), {
    delete Module.kumir2PickedFiles;
});

EM_JS(int, kumir2HasSaveFilePicker, (), {
    return typeof window.showSaveFilePicker === "function";
});

// Lets the user choose a file to save to. The file handle is kept for later
// writes. Result: its id (> 0), 0 on cancel, -1 on failure; the chosen name
// is in Module.kumir2SaveName.
EM_JS(void, kumir2StartChooseSaveFile, (const char *suggestedNameUtf8, const char *acceptUtf8), {
    const operation = Module.kumir2Operation = { done: false, result: -1 };
    const accept = UTF8ToString(acceptUtf8);
    const options = { suggestedName: UTF8ToString(suggestedNameUtf8) };
    if (accept)
        options.types = [{ accept: { "application/octet-stream": accept.split(",") } }];
    const finish = result => {
        operation.result = result;
        operation.done = true;
    };
    try {
        window.showSaveFilePicker(options).then(handle => {
            Module.kumir2SaveHandles = Module.kumir2SaveHandles || { next: 1 };
            const id = Module.kumir2SaveHandles.next++;
            Module.kumir2SaveHandles[id] = handle;
            Module.kumir2SaveName = new TextEncoder().encode(handle.name || options.suggestedName);
            finish(id);
        }, error => {
            if (error && error.name === "AbortError")
                return finish(0);
            console.warn("Browser file save failed", error);
            finish(-1);
        });
    } catch (error) {
        console.warn("Browser file save failed", error);
        finish(-1);
    }
});

EM_JS(int, kumir2SaveNameSize, (), {
    return Module.kumir2SaveName.length;
});

EM_JS(void, kumir2SaveNameCopy, (char *out), {
    HEAPU8.set(Module.kumir2SaveName, out);
    delete Module.kumir2SaveName;
});

// Writes @param data to the file chosen by kumir2StartChooseSaveFile.
// Result: 1 on success, -1 on failure.
EM_JS(void, kumir2StartWriteSaveFile, (int id, const char *data, int size), {
    const operation = Module.kumir2Operation = { done: false, result: -1 };
    const bytes = HEAPU8.slice(data, data + size);
    const write = async () => {
        const stream = await Module.kumir2SaveHandles[id].createWritable();
        await stream.write(bytes);
        await stream.close();
        return 1;
    };
    write().then(result => {
        operation.result = result;
        operation.done = true;
    }, error => {
        console.warn("Browser file save failed", error);
        operation.done = true;
    });
});

EM_JS(int, kumir2OperationDone, (), {
    return Module.kumir2Operation.done ? 1 : 0;
});

EM_JS(int, kumir2OperationResult, (), {
    return Module.kumir2Operation.result;
});

namespace
{

// Waits for the browser operation started last, returns its result
int waitForBrowser()
{
	if (!kumir2OperationDone()) {
		QEventLoop loop;
		QTimer poll;
		QObject::connect(&poll, &QTimer::timeout, &loop, [&loop]() {
			if (kumir2OperationDone()) {
				loop.quit();
			}
		});
		poll.start(20);
		// The browser picker is modal: user input waits until it is closed
		loop.exec(QEventLoop::ExcludeUserInputEvents);
	}
	return kumir2OperationResult();
}

struct BrowserSaveTarget {
	int handle; // browser file handle id; 0: no save picker, the file is downloaded
	bool delivered;
};

// Directories created by getSaveFileName -> where their files are saved
QHash<QString, BrowserSaveTarget> &browserSaveTargets()
{
	static QHash<QString, BrowserSaveTarget> targets;
	return targets;
}

// Patterns of a QFileDialog filter: "Programs (*.kum);;All files (*)" -> "*.kum", "*"
QStringList filterPatterns(const QString &filter)
{
	QStringList result;
	const QStringList entries = filter.split(";;");
	for (const QString &entry : entries) {
		QString patterns = entry;
		const int open = entry.indexOf('(');
		const int close = entry.lastIndexOf(')');
		if (open >= 0 && close > open) {
			patterns = entry.mid(open + 1, close - open - 1);
		}
		result += patterns.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
	}
	return result;
}

// Extensions for browser pickers: ".kum,.txt", or empty for any file.
// Browsers match only the last extension (".kurs.xml" -> ".xml").
QString browserAccept(const QString &filter)
{
	static const QRegularExpression validExtension("^[A-Za-z0-9+_-]{1,15}$");
	QStringList extensions;
	for (const QString &pattern : filterPatterns(filter)) {
		if (pattern == "*" || pattern == "*.*") {
			return QString();
		}
		const QString extension = pattern.mid(pattern.lastIndexOf('.') + 1);
		if (pattern.contains('.') && validExtension.match(extension).hasMatch()
			&& !extensions.contains("." + extension)) {
			extensions << "." + extension;
		}
	}
	return extensions.join(",");
}

// Relative path of a chosen file without "..", "." and empty parts
QString pickedName(int index)
{
	QByteArray name(kumir2PickedNameSize(index), '\0');
	kumir2PickedNameCopy(index, name.data());
	QStringList parts;
	const QStringList rawParts = QString::fromUtf8(name).split(QRegularExpression("[/\\\\]"));
	for (const QString &part : rawParts) {
		if (!part.isEmpty() && part != "." && part != "..") {
			parts << part;
		}
	}
	return parts.join("/");
}

QByteArray pickedData(int index)
{
	QByteArray data(kumir2PickedDataSize(index), '\0');
	if (!data.isEmpty()) {
		kumir2PickedDataCopy(index, data.data());
	}
	return data;
}

QString newTemporaryDirectory(const QString &kind)
{
	QTemporaryDir directory(QDir::tempPath() + "/kumir2-" + kind + "-XXXXXX");
	if (!directory.isValid()) {
		return QString();
	}
	directory.setAutoRemove(false);
	return directory.path();
}

enum PickMode { PickFile = 0, PickFiles = 1, PickDirectory = 2 };

// Copies files chosen in the browser to one new directory @param uploadDirectory,
// returns their paths there
QStringList openBrowserFiles(QWidget *parent, const QString &filter, PickMode mode,
	QString *uploadDirectory = 0)
{
	kumir2StartPickFiles(browserAccept(filter).toUtf8().constData(), mode);
	const int count = waitForBrowser();
	if (count <= 0) {
		if (count < 0) {
			QMessageBox::critical(parent, Widgets::FileDialog::tr("Can't open file"),
				Widgets::FileDialog::tr("The browser could not open the file."));
		}
		return QStringList();
	}
	QStringList result;
	const QString directory = newTemporaryDirectory("upload");
	if (directory.isEmpty()) {
		kumir2PickedClear();
		QMessageBox::critical(parent, Widgets::FileDialog::tr("Can't open file"),
			Widgets::FileDialog::tr("Can't create temporary directory"));
		return result;
	}
	for (int i = 0; i < count; ++i) {
		QString name = pickedName(i);
		if (name.isEmpty()) {
			name = "file";
		}
		const QString path = directory + "/" + name;
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile file(path);
		const QByteArray data = pickedData(i);
		if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size()) {
			QMessageBox::critical(parent, Widgets::FileDialog::tr("Can't open file"), file.errorString());
			result.clear();
			break;
		}
		file.close();
		result << file.fileName();
	}
	kumir2PickedClear();
	if (uploadDirectory) {
		*uploadDirectory = directory;
	}
	return result;
}

// Opens the browser save picker. Returns the file handle id (> 0),
// 0 on cancel, -1 on failure; @param name gets the chosen file name.
int chooseBrowserSaveFile(QString &name, const QString &filter)
{
	kumir2StartChooseSaveFile(name.toUtf8().constData(), browserAccept(filter).toUtf8().constData());
	const int handle = waitForBrowser();
	if (handle > 0) {
		QByteArray chosenName(kumir2SaveNameSize(), '\0');
		kumir2SaveNameCopy(chosenName.data());
		const QString chosen = QFileInfo(QString::fromUtf8(chosenName)).fileName();
		if (!chosen.isEmpty()) {
			name = chosen;
		}
	}
	return handle;
}

bool writeBrowserSaveFile(int handle, const QByteArray &contents)
{
	kumir2StartWriteSaveFile(handle, contents.constData(), contents.size());
	return waitForBrowser() > 0;
}

QString suggestedSaveName(const QString &dir, const QString &filter)
{
	const QFileInfo info(dir);
	QString name = (dir.isEmpty() || dir.endsWith('/') || info.isDir())
		? QString("untitled") : info.fileName();
	if (!name.contains('.')) {
		for (const QString &pattern : filterPatterns(filter)) {
			if (pattern.startsWith("*.") && pattern != "*.*") {
				name += pattern.mid(1);
				break;
			}
		}
	}
	return name;
}

} // namespace
#endif // KUMIR2_BROWSER_FILE_DIALOGS

namespace Widgets
{

QString FileDialog::getOpenFileName(QWidget *parent, const QString &caption,
	const QString &dir, const QString &filter)
{
#ifdef KUMIR2_BROWSER_FILE_DIALOGS
	Q_UNUSED(caption);
	Q_UNUSED(dir);
	const QStringList files = openBrowserFiles(parent, filter, PickFile);
	return files.isEmpty() ? QString() : files.first();
#else
	return QFileDialog::getOpenFileName(parent, caption, dir, filter);
#endif
}

QStringList FileDialog::getOpenFileNames(QWidget *parent, const QString &caption,
	const QString &dir, const QString &filter)
{
#ifdef KUMIR2_BROWSER_FILE_DIALOGS
	Q_UNUSED(caption);
	Q_UNUSED(dir);
	return openBrowserFiles(parent, filter, PickFiles);
#else
	return QFileDialog::getOpenFileNames(parent, caption, dir, filter);
#endif
}

QString FileDialog::getExistingDirectory(QWidget *parent, const QString &caption,
	const QString &dir)
{
#ifdef KUMIR2_BROWSER_FILE_DIALOGS
	Q_UNUSED(caption);
	Q_UNUSED(dir);
	QString directory;
	const QStringList files = openBrowserFiles(parent, QString(), PickDirectory, &directory);
	if (files.isEmpty()) {
		return QString();
	}
	// Each file is <directory>/<chosen folder name>/...
	const QString folder = files.first().mid(directory.length() + 1).section('/', 0, 0);
	return directory + "/" + folder;
#else
	return QFileDialog::getExistingDirectory(parent, caption, dir);
#endif
}

QString FileDialog::getSaveFileName(QWidget *parent, const QString &caption,
	const QString &dir, const QString &filter)
{
#ifdef KUMIR2_BROWSER_FILE_DIALOGS
	QString name = suggestedSaveName(dir, filter);
	int handle = 0;
	if (kumir2HasSaveFilePicker()) {
		handle = chooseBrowserSaveFile(name, filter);
		if (handle <= 0) {
			if (handle < 0) {
				QMessageBox::critical(parent, tr("Can't save file"),
					tr("The browser could not save the file."));
			}
			return QString();
		}
	} else {
		// The file will be downloaded: the browser does not tell where to
		// or whether the user cancels, so only the name is asked here
		bool ok = false;
		name = QInputDialog::getText(parent, caption.isEmpty() ? tr("Save file") : caption,
			tr("File name:"), QLineEdit::Normal, name, &ok).trimmed();
		name = QFileInfo(name).fileName();
		if (!ok || name.isEmpty()) {
			return QString();
		}
	}
	const QString directory = newTemporaryDirectory("save");
	if (directory.isEmpty()) {
		QMessageBox::critical(parent, tr("Can't save file"), tr("Can't create temporary directory"));
		return QString();
	}
	BrowserSaveTarget target = { handle, false };
	browserSaveTargets().insert(directory, target);
	return directory + "/" + name;
#else
	return QFileDialog::getSaveFileName(parent, caption, dir, filter);
#endif
}

bool FileDialog::finishSave(QWidget *parent, const QString &path, Delivery delivery)
{
#ifdef KUMIR2_BROWSER_FILE_DIALOGS
	const QFileInfo info(path);
	const QString directory = info.absolutePath();
	if (!browserSaveTargets().contains(directory)) {
		return true;
	}
	const BrowserSaveTarget target = browserSaveTargets().value(directory);
	if (target.handle == 0 && delivery == Update && target.delivered) {
		return true;
	}
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::critical(parent, tr("Can't save file"), file.errorString());
		return false;
	}
	const QByteArray contents = file.readAll();
	file.close();
	if (target.handle > 0) {
		// The same browser file is written again on later saves to this path
		if (!writeBrowserSaveFile(target.handle, contents)) {
			QMessageBox::critical(parent, tr("Can't save file"),
				tr("The browser could not save the file."));
			return false;
		}
	} else {
		QFileDialog::saveFileContent(contents, info.fileName(), parent);
	}
	browserSaveTargets()[directory].delivered = true;
	return true;
#else
	Q_UNUSED(parent);
	Q_UNUSED(path);
	Q_UNUSED(delivery);
	return true;
#endif
}

FileDialog::SaveResult FileDialog::saveFileContent(QWidget *parent, const QByteArray &contents,
	const QString &suggestedName, const QString &filter)
{
#if defined(KUMIR2_BROWSER_FILE_DIALOGS)
	Q_UNUSED(parent);
	if (kumir2HasSaveFilePicker()) {
		QString name = suggestedName;
		const int handle = chooseBrowserSaveFile(name, filter);
		if (handle <= 0) {
			return handle == 0 ? Cancelled : Failed;
		}
		return writeBrowserSaveFile(handle, contents) ? Saved : Failed;
	}
	QFileDialog::saveFileContent(contents, suggestedName, parent);
	return Downloaded;
#elif defined(Q_OS_WASM)
	Q_UNUSED(filter);
	QFileDialog::saveFileContent(contents, suggestedName, parent);
	return Downloaded;
#else
	const QString fileName = QFileDialog::getSaveFileName(parent, QString(), suggestedName, filter);
	if (fileName.isEmpty()) {
		return Cancelled;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) {
		return Failed;
	}
	file.close();
	return Saved;
#endif
}

} // namespace Widgets
