#include "Logger.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>

static const char* LevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO ";
    case LogLevel::Warning: return "WARN ";
    case LogLevel::Error:   return "ERROR";
    default:                return "?????";
    }
}

static std::string FormatMessage(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func)
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    std::string_view filename = file;
    auto pos = filename.find_last_of("/\\");
    if (pos != std::string_view::npos)
        filename = filename.substr(pos + 1);

    std::ostringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S")
        << " [" << LevelToString(level) << "]"
        << " [" << system << "] "
        << msg
        << "  (" << func << " @ " << filename << ":" << line << ")";

    return ss.str();
}

void Logger::AddSink(std::shared_ptr<LogInternals::Sink> sink)
{
    _sinks.push_back(sink);
}

void Logger::RemoveSink(std::shared_ptr<LogInternals::Sink> sink)
{
    _sinks.erase(std::remove(_sinks.begin(), _sinks.end(), sink), _sinks.end());
}

void Logger::Log(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func)
{
    if (level < minLevel)
        return;

    for (auto& sink : _sinks)
        sink->Write(level, system, msg, file, line, func);
}

void ConsoleSink::Write(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func)
{
    std::string formatted = FormatMessage(level, system, msg, file, line, func);

    if (level == LogLevel::Error || level == LogLevel::Warning)
        std::cerr << formatted << '\n';
    else
        std::cout << formatted << '\n';
}

FileSink::FileSink(std::string_view path)
{
    _file.open(std::string(path), std::ios::out | std::ios::trunc);
}

void FileSink::Write(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func)
{
    if (!_file.is_open())
        return;

    _file << FormatMessage(level, system, msg, file, line, func) << '\n';
    _file.flush();
}