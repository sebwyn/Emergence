#include "Logger.hpp"

#include <fstream>
#include <iostream>

std::string Logger::logFile = "log.txt";
std::string Logger::relativeDir = "Emergence/";

void Logger::create(const std::string &_logFile,
                    const std::string &_relativeDir) {
    logFile = _logFile;
    relativeDir = _relativeDir;
}

void Logger::logInfo(const std::string &message) {

    std::ofstream log;
    log.open(logFile, std::ios::app);
    // filename
    log << " [I]: " << message << std::endl;
    log.close();
}

/*void Logger::logInfo(const std::string &message,
                     const std::source_location location) {

    std::ofstream log;
    log.open(logFile, std::ios::app);
    // filename
    std::string filename = location.file_name();
    filename =
        filename.substr(filename.find(relativeDir) + relativeDir.length());
    log << filename << ":" << location.line() << " [I]: " << message
        << std::endl;
    log.close();
}*/

void Logger::logError(const std::string &message) {
    std::ofstream log;
    log.open(logFile, std::ios::app);
    log << "[I]: " << message << std::endl;
    log.close();
}
