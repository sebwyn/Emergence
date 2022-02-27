#include "Logger.hpp"

#include <fstream>
#include <iostream>

std::string Logger::logFile = "log.txt";

void Logger::create(const std::string &_logFile) { logFile = _logFile; }

void Logger::logInfo(const std::string &message) {
    std::ofstream log;
    log.open(logFile, std::ios::app);
    log << "[I]: " << message << std::endl;
    log.close();
}

void Logger::logError(const std::string &message) {
    std::ofstream log;
    log.open(logFile, std::ios::app);
    log << "[I]: " << message << std::endl;
    log.close();
}
