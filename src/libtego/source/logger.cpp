#ifdef ENABLE_TEGO_LOGGER

//
// logger methods
//

void logger::trace(const source_location& loc)
{
    println("{}:{} -> {}(...)", loc.file_name(), loc.line(), loc.function_name());
}

std::ofstream& logger::get_stream()
{
    static std::ofstream fs("libtego.log", std::ios::binary);
    return fs;
}

std::mutex& logger::get_mutex()
{
    static std::mutex m;
    return m;
}

double logger::get_timestamp()
{
    const static auto start = std::chrono::system_clock::now();
    const auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> duration(now - start);
    return duration.count();
}
#endif

