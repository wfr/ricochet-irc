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

// fmt
#include <fmt/format.h>
#include <fmt/ostream.h>

// Qt

#include <QDateTime>
#include <QDir>
#ifdef ENABLE_GUI
	#include <QClipboard>
	#include <QFileDialog>
	#include <QGuiApplication>
	#include <QMessageBox>
	#include <QPushButton>
	#include <QQmlApplicationEngine>
	#include <QQmlContext>
	#include <QQmlEngine>
	#include <QQmlNetworkAccessManagerFactory>
	#include <QQuickItem>
	#include <QRegularExpressionValidator>
	#include <QScreen>
	#include <QtQml>
#else
	#include "QRegularExpressionValidator.h"
#endif
#ifdef Q_OS_MAC
#   include <QtMac>
#endif // Q_OS_MAC

// tego
#include <tego/tego.hpp>

#endif // __cplusplus


