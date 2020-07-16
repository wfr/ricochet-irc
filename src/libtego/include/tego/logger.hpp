#pragma once

#ifdef ENABLE_TEGO_LOGGER

// std
#include <memory>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <mutex>
#include <typeinfo>
#include <experimental/source_location>
using std::experimental::source_location;

// fmt
#include <fmt/format.h>
#include <fmt/ostream.h>

// wrapper around fmt::print that writes to singleton log file libtego.log
class logger
{
public:
    template<size_t N, typename... ARGS>
    static void println(const char (&format)[N], ARGS&&... args)
    {
        std::lock_guard<std::mutex> guard(get_mutex());

        auto& fs = get_stream();

        fmt::print(fs, "[{:f}] ", get_timestamp());
        fmt::print(fs, format, std::forward<ARGS>(args)...);
        fs << std::endl;
    }

    template<size_t N>
    static void println(const char (&msg)[N])
    {
        std::lock_guard<std::mutex> guard(get_mutex());

        auto& fs = get_stream();

        fmt::print(fs, "[{:f}] ", get_timestamp());
        fs << msg << std::endl;
    }

    static void trace(const source_location& loc = source_location::current());
private:
    static std::ofstream& get_stream();
    static std::mutex& get_mutex();
    static double get_timestamp();
};

std::ostream& operator<<(std::ostream& out, const class QString& str);
std::ostream& operator<<(std::ostream& out, const class QByteArray& blob);
std::ostream& operator<<(std::ostream& out, const std::type_info& ti);
#else // ENABLE_TEGO_LOGGER

// mock no-op logger
class logger
{
public:
    template<size_t N, typename... ARGS>
    static void println(const char (&)[N], ARGS&&...) {}
    template<size_t N>
    static void println(const char (&)[N]) {}
    static void trace() {}
};



#endif // ENABLE_TEGO_LOGGER