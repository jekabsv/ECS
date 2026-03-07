#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <fstream>

enum class LogLevel { Debug, Info, Warning, Error };

namespace LogInternals
{
    class Sink
    {
    public:
        virtual void Write(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func) = 0;
        virtual ~Sink() = default;
    };
}

class ConsoleSink : public LogInternals::Sink
{
public:
    void Write(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func) override;
};

class FileSink : public LogInternals::Sink
{
public:
    FileSink(std::string_view path);
    void Write(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func) override;
private:
    std::ofstream _file;
};

class Logger
{
public:
    LogLevel minLevel = LogLevel::Debug;

    void AddSink(std::shared_ptr<LogInternals::Sink> sink);
    void RemoveSink(std::shared_ptr<LogInternals::Sink> sink);
    void Log(LogLevel level, std::string_view system, std::string_view msg, std::string_view file, int line, std::string_view func);

private:
    std::vector<std::shared_ptr<LogInternals::Sink>> _sinks;
};

Logger& GlobalLogger();

#ifdef NDEBUG
#define LOG_DEBUG(logger, system, msg) ((void)0)
#define LOG_INFO(logger, system, msg)  ((void)0)
#else
#define LOG_DEBUG(logger, system, msg) (logger).Log(LogLevel::Debug,   system, msg, __FILE__, __LINE__, __func__)
#define LOG_INFO(logger, system, msg)  (logger).Log(LogLevel::Info,    system, msg, __FILE__, __LINE__, __func__)
#endif

#define LOG_WARN(logger, system, msg)  (logger).Log(LogLevel::Warning, system, msg, __FILE__, __LINE__, __func__)
#define LOG_ERROR(logger, system, msg) (logger).Log(LogLevel::Error,   system, msg, __FILE__, __LINE__, __func__)