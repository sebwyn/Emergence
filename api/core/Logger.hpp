#pragma once

#include <memory>
#include <string>
#include <source_location>

class Logger {
  public:
    static void create(const std::string &logfile, const std::string &_relativeDir);

    static void logInfo(
        const std::string &message,
        const std::source_location location = std::source_location::current());
    static void logError(const std::string &message);

  private:
    static std::string logFile;
    static std::string relativeDir;
};
