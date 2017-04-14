#ifndef TESTGUI_GLOBAL_H
#define TESTGUI_GLOBAL_H

#include <QtGlobal>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(testgui)

#if defined(TESTGUI_LIBRARY)
#  define TESTGUISHARED_EXPORT Q_DECL_EXPORT
#else
#  define TESTGUISHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // TESTGUI_GLOBAL_H
