/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <QApplication>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDebug>

#include <KAboutData>
#include <KLocalizedString>
#include <KDBusService>

#include "SpectacleCore.h"
#include "SpectacleDBusAdapter.h"
#include "ExportManager.h"
#include "Config.h"

int main(int argc, char **argv)
{
    // set up the application

    QApplication app(argc, argv);

    app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KLocalizedString::setApplicationDomain("spectacle");

    KAboutData aboutData(QStringLiteral("spectacle"),
                         i18n("Spectacle"),
                         QStringLiteral(SPECTACLE_VERSION) + QStringLiteral(" - ") + QStringLiteral(SPECTACLE_CODENAME),
                         i18n("KDE Screenshot Utility"),
                         KAboutLicense::GPL_V2,
                         i18n("(C) 2015 Boudhayan Gupta"));
    aboutData.addAuthor(QStringLiteral("Boudhayan Gupta"), QString(), QStringLiteral("bgupta@kde.org"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("spectacle")));

    // set up the command line options parser

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.addOptions({
        {{QStringLiteral("f"), QStringLiteral("fullscreen")},        i18n("Capture the entire desktop (default)")},
        {{QStringLiteral("m"), QStringLiteral("current")},           i18n("Capture the current monitor")},
        {{QStringLiteral("a"), QStringLiteral("activewindow")},      i18n("Capture the active window")},
        {{QStringLiteral("u"), QStringLiteral("windowundercursor")}, i18n("Capture the window currently under the cursor, including parents of pop-up menus")},
        {{QStringLiteral("t"), QStringLiteral("transientonly")},     i18n("Capture the window currently under the cursor, excluding parents of pop-up menus")},
        {{QStringLiteral("r"), QStringLiteral("region")},            i18n("Capture a rectangular region of the screen")},
        // Parser option to start in GUI mode removed as redundant, as that's the default mode
        {{QStringLiteral("n"), QStringLiteral("nonotify")},          i18n("Take a screenshot and save it with an automatic filename, with no notification or GUI.")},
        {{QStringLiteral("o"), QStringLiteral("output")},            i18n("Take a screenshot and save it to the specified filename."), QStringLiteral("filename")},
        // "delay" input changed to seconds instead of milliseconds (more user-friendly)
        {{QStringLiteral("d"), QStringLiteral("delay")},             i18n("Delay before automatically taking the shot (in seconds)"), QStringLiteral("delaySeconds")},
        {{QStringLiteral("w"), QStringLiteral("onclick")},           i18n("Wait for a mouse click before taking screenshot.")},
        // DBus option moved to bottom of help output, since it's the most obscure
        {{QStringLiteral("s"),QStringLiteral("dbus")},               i18n("Start in DBus-Activation mode")},
    });

    parser.process(app);
    aboutData.processCommandLine(&parser);

    // extract the capture mode

    ImageGrabber::GrabMode grabMode = ImageGrabber::FullScreen;
    if (parser.isSet(QStringLiteral("current"))) {
        grabMode = ImageGrabber::CurrentScreen;
    } else if (parser.isSet(QStringLiteral("activewindow"))) {
        grabMode = ImageGrabber::ActiveWindow;
    } else if (parser.isSet(QStringLiteral("region"))) {
        grabMode = ImageGrabber::RectangularRegion;
    } else if (parser.isSet(QStringLiteral("windowundercursor"))) {
        grabMode = ImageGrabber::TransientWithParent;
    } else if (parser.isSet(QStringLiteral("transientonly"))) {
        grabMode = ImageGrabber::WindowUnderCursor;
    }

    // are we running in background or dbus mode?

    SpectacleCore::StartMode startMode = SpectacleCore::GuiMode;
    bool notify = true;
    qint64 delayMsec = 0;
    QString fileName = QString();


    // BackgroundMode varieties...

    if (parser.isSet(QStringLiteral("dbus"))) {
        startMode = SpectacleCore::DBusMode;
        app.setQuitOnLastWindowClosed(false);
    } else if (parser.isSet(QStringLiteral("nonotify"))) {
        startMode = SpectacleCore::BackgroundMode;
        notify = false;
    } else if (parser.isSet(QStringLiteral("output"))) {
        startMode = SpectacleCore::BackgroundMode;
        fileName = parser.value(QStringLiteral("output"));
        notify = false;
    } else if (parser.isSet(QStringLiteral("delay"))) {
        bool ok;
        startMode = SpectacleCore::BackgroundMode;
        // Human-friendly seconds back to computer-friendly milliseconds
        delayMsec = (parser.value(QStringLiteral("delay")).toLongLong(&ok) * 1000);
        // ok variable automatically set by string->int conversion (handy!)
        if (!ok) {
            QTextStream(stdout) << i18n("ERROR: Invalid value for ") << "<delaySeconds>\n" << endl;
            app.exit();
        }
    } else if (parser.isSet(QStringLiteral("onclick"))) {
            startMode = SpectacleCore::BackgroundMode;
            delayMsec = -1;
    };



    // (re)release the kraken (with a version number update)

    SpectacleCore core(startMode, grabMode, fileName, delayMsec, notify);
    QObject::connect(&core, &SpectacleCore::allDone, qApp, &QApplication::quit);

    // create the dbus connections

    new KDBusService(KDBusService::Multiple, &core);

    SpectacleDBusAdapter *dbusAdapter = new SpectacleDBusAdapter(&core);
    QObject::connect(&core, &SpectacleCore::grabFailed, dbusAdapter, &SpectacleDBusAdapter::ScreenshotFailed);
    QObject::connect(ExportManager::instance(), &ExportManager::imageSaved, [&](const QUrl savedAt) {
        emit dbusAdapter->ScreenshotTaken(savedAt.toLocalFile());
    });

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), &core);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Spectacle"));

    // fire it up

    return app.exec();
}
