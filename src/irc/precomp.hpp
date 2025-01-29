#include <QtGlobal>

// C headers

// standard library
#include <limits.h>

// C++ headers
#ifdef __cplusplus

// standard library
#include <sstream>
#include <iomanip>
#include <cassert>
#include <type_traits>
#include <cstdint>
#include <functional>
#include <fstream>
#include <iterator>
#include <set>
#include <random>

// fmt
#include <fmt/format.h>
#include <fmt/ostream.h>

// Qt
#include <QAbstractListModel>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLockFile>
#include <QPointer>
#include <QRegularExpression>
#ifdef ENABLE_GUI
#include <QRegularExpressionValidator> // QtGui
#endif
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#ifdef Q_OS_MAC
#   include <QtMac>
#endif // Q_OS_MAC

// tego
#include <tego/tego.hpp>

#ifdef TEGO_VERSION
#   define TEGO_STR2(X) #X
#   define TEGO_STR(X) TEGO_STR2(X)
#   define TEGO_VERSION_STR TEGO_STR(TEGO_VERSION)
#else
#   define TEGO_VERSION_STR "devbuild"
#endif

#endif // __cplusplus


