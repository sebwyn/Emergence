#pragma once

#include <memory>
#include <string>

class Logger {
  public:
    static void create(const std::string &logfile);

    static void logInfo(const std::string &message);
    static void logError(const std::string &message);

  private:
    static std::string logFile;
};
