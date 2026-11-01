#ifndef KUMIRCODERUN_PLUGIN_H
#define KUMIRCODERUN_PLUGIN_H

#include <kumir2-libs/extensionsystem/kplugin.h>
#include <kumir2/runinterface.h>
#include <QDateTime>


namespace Kumir
{
class AbstractInputBuffer;
class AbstractOutputBuffer;
}

namespace KumirCodeRun
{

namespace Gui {
class SimulatedInputBuffer;
class SimulatedOutputBuffer;
}


class KumirRunPlugin :
	public ExtensionSystem::KPlugin,
	public Shared::RunInterface
{
	Q_OBJECT
#if QT_VERSION >= 0x050000
#ifndef KUMIR2_STATIC_PLUGINS // static build registers plugins by factory functions
	Q_PLUGIN_METADATA(IID "kumir2.KumirCodeRun")
#endif
#endif
	Q_INTERFACES(Shared::RunInterface)

public:
	explicit KumirRunPlugin();
	~KumirRunPlugin();

	RunMode currentRunMode() const;
	bool canStepOut() const;
	void terminateAndWaitForStopped();
	bool loadProgram(const RunnableProgram &sourceInfo);
	bool hasMoreInstructions() const;
	bool hasTestingEntryPoint() const;
	bool hasBreakpointsSupport() const;
	QString error() const;

	int currentLineNo() const ;
	QPair<quint32, quint32> currentColumn() const;
	QDateTime loadedProgramVersion() const { return loadedVersion_; }
	unsigned long int stepsCounted() const;
	QAbstractItemModel *debuggerVariablesViewModel() const;

	bool isGuiRequired() const { return false; }
	QVariant valueStackTopItem() const;

	QMap<QString, QVariant> getScalarLocalValues(int frameNo) const;
	QMap<QString, QVariant> getScalarGlobalValues(const QString &moduleName) const;
	QVariantList getLocalTableValues(
		int frameNo,
		int maxCount,
		const QString &name,
		const QList< QPair<int, int> > &ranges,
		bool &complete
	) const;
	QVariantList getGlobalTableValues(
		const QString &moduleName,
		int maxCount,
		const QString &name,
		const QList< QPair<int, int> > &ranges,
		bool &complete
	) const;

	QVariant getLocalTableValue(
		int frameNo,
		const QString &name,
		const QList<int> &indeces
	) const;

	QVariant getGlobalTableValue(
		const QString &moduleName,
		const QString &name,
		const QList<int> &indeces
	) const;

	bool isTestingRun() const;
	void updateSettings(const QStringList &) {}

	void setStdInTextStream(QTextStream *);
	void setStdOutTextStream(QTextStream *);

public slots:
	void runProgramInCurrentThread(bool useTestingEntryPoint = false);
	void runBlind();
	void runContinuous();
	void runStepOver();
	void runStepInto();
	void runToEnd();
	void runTesting();
	void terminate();

	void removeAllBreakpoints();
	void insertOrChangeBreakpoint(
		bool enabled,
		const QString &fileName,
		quint32 lineNo,
		quint32 ignoreCount,
		const QString &condition
	);
	void insertSingleHitBreakpoint(const QString &fileName, quint32 lineNo);
	void removeBreakpoint(const QString &fileName, quint32 lineNo);

protected:
	void createPluginSpec();
	void start();
	void stop();
	void timerEvent(QTimerEvent *);
	QList<ExtensionSystem::CommandLineParameter> acceptableCommandLineParameters() const;
	QString initialize(const QStringList &configurationArguments,
		const ExtensionSystem::CommandLine &runtimeArguments);
	void prepareCommonRun();
	void prepareConsoleRun();
	void prepareGuiRun();

private:
	QDateTime loadedVersion_;
	bool done_;
	bool onlyOneTryToInput_;

protected slots:
	void handleThreadFinished();
	void handleLineChanged(int lineNo, quint32 colStart, quint32 colEnd);
	void handleBreakpointHit(const QString &fileName, int lineNo);
	void checkForErrorInConsole();

signals:
	void updateStepsCounter(quint64);
	void errorOutputRequest(const QString &);
	void stopped(int reason);
	void finishInput(const QVariantList &data);
	void inputRequest(const QString &format);
	void outputRequest(const QString &output);
	void lineChanged(int lineNo, quint32 colStart, quint32 colEnd);
	void marginText(int lineNo, const QString &text);
	void replaceMarginText(int lineNo, const QString &text, bool redFgColor);
	void clearMargin(int fromLine, int toLine);
	void resetModule(const QString &actorPluginName);
	void showActorWindowRequest(const QByteArray &asciiModuleName);

private:
	class Run *pRun_;
	struct CommonFunctors *common_;
	struct ConsoleFunctors *console_;
	struct GuiFunctors *gui_;

	Gui::SimulatedInputBuffer *simulatedInputBuffer_;
	Kumir::AbstractInputBuffer *defaultInputBuffer_;

	Gui::SimulatedOutputBuffer *simulatedOutputBuffer_;
	Kumir::AbstractOutputBuffer *defaultOutputBuffer_;
};

} // namespace KumirCodeRun

#endif // KUMIRCODERUN_PLUGIN_H
