// Run twice in the same browser context (reload the page) to test persistence.
#include <QCoreApplication>
#include <QSettings>
#include <cstdio>

static void probe(const char *name, QSettings::Format format)
{
    QSettings settings(format, QSettings::UserScope, "kumir2-wasm-probe", name);
    const int previous = settings.value("reloadCount", 0).toInt();
    settings.setValue("reloadCount", previous + 1);
    settings.sync();
    std::printf("settings.%s: previous=%d current=%d status=%d file=%s\n",
                name, previous, settings.value("reloadCount").toInt(),
                int(settings.status()), qPrintable(settings.fileName()));
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    probe("native", QSettings::NativeFormat);
    probe("localstorage", QSettings::WebLocalStorageFormat);
    return 0;
}
