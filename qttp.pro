contains(CONFIG, SAMPLEAPP) {
    message(********* Building SAMPLE APPLICATION qttpserver *********)
    # This default configuration is for example purposes.
    TEMPLATE = app
    DESTDIR = $$PWD
    SOURCES += $$PWD/examples/sample/main.cpp
    TARGET = QttpServer
    macx {
        # Since things are buried in the app folder, we'll copy configs there.

        Config.files = $$PWD/config/global.json $$PWD/config/routes.json
        Config.path = Contents/MacOS/config
        QMAKE_BUNDLE_DATA += Config
    }
    message('Including config files')
    include($$PWD/config/config.pri)

} else {

    TEMPLATE = lib
    TARGET = qttpserver

    !win32 {
        VERSION = 0.0.1
    }

    DEFINES += QTTP_LIBRARY QTTP_EXPORT

    contains(CONFIG, QTTP_SHARED_LIBRARY) {
        message(********* Building shared library qttpserver *********)
        unix {
            #target.path = /usr/lib
            #INSTALLS += target
        }

    } else {
        message(********* Building static library qttpserver *********)
        CONFIG += staticlib
    }

    CONFIG(debug, debug|release) {
      DESTDIR = $$top_builddir/debug
    }
    CONFIG(release, debug|release) {
      DESTDIR = $$top_builddir/release
    }
}

QT -= gui

message('Including core files')
include($$PWD/core.pri)
