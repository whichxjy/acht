#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include "SyncQueue.hpp"
#include <string>
#include <fstream>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <atomic>

namespace acht {

    class Logger {
    public:
        using LogRecord = std::string;
        using LogMessage = std::string;

        /***********************************************************
         *  The level of the events to be tracked (or the level of
         *  messages to be sent).
         ***********************************************************/
        enum class Level {
            FATAL,
            ERROR,
            WARN,
            INFO,
            DEBUG
        };

        /***********************************************************
         *  Get the instance of logger. It allows only a single
         *  instance to be created. If the parameter "level" is
         *  different form logger's level, reset it.
         ***********************************************************/
        static std::shared_ptr<Logger> getLogger(Level level = Level::DEBUG) {
            if (my_logger == nullptr) {
                my_logger = std::shared_ptr<Logger>(new Logger(level));
            }
            else if (my_logger->my_level != level) {
                my_logger->setLevel(level);
            }
            return my_logger;
        }

        /***********************************************************
         *  Destroy the logger.
         ***********************************************************/
        static void destroyLogger() {
            my_logger = nullptr;
        }

        /***********************************************************
         *  Destructor.
         ***********************************************************/
        ~Logger() {
            // Stop the logger and close the file stream.
            stop();
            if (log_file_stream) {
                log_file_stream->close();
                log_file_stream = nullptr;
            }
        }

        /***********************************************************
         *  Add log record to the log queue.
         ***********************************************************/
        void write(Level level, const LogMessage& log_msg) {
            if (level > my_level)
                return;

            // Create log record
            std::stringstream logRecordStream;
            logRecordStream << getCurrentTime()
                << " [" << levelToString(level) << "] "
                << log_msg;

            // Add the log record to log queue
            log_queue.put(logRecordStream.str());
        }

        /***********************************************************
         *  Set the threshold for this logger. Logging messages which
         *  are less severe than this level will be ignored.
         ***********************************************************/
        void setLevel(Level level) {
            my_level = level;
        }

        /***********************************************************
         *  Get the threshold for this logger.
         ***********************************************************/
        Level getLevel() const {
            return my_level;
        }

        /***********************************************************
         *  Get the threshold for this logger as a string.
         ***********************************************************/
        std::string getLevelString() const {
            return levelToString(my_level);
        }

        /***********************************************************
         *  Stop the logger.
         ***********************************************************/
        void stop() {
            if (!need_to_stop) {
                need_to_stop = true;

                // Stop the log queue
                log_queue.stop();

                // Wait until all log records are written to file
                write_thread->join();
                write_thread = nullptr;
            }
        }

        /***********************************************************
         *  If the logger was stopped, restart it.
         ***********************************************************/
        void start() {
            if (need_to_stop) {
                need_to_stop = false;
                write_thread = std::make_shared<std::thread>([this] { runWriteThread(); } );
                log_queue.start();
            }
        }

        /***********************************************************
         *  Set the log file path where the log records are send to.
         ***********************************************************/
        bool setLogFilePath(const std::string& log_file_path) {
            std::lock_guard<std::mutex> lock(my_mutex);
            if (my_log_file_path == log_file_path) {
                return true;
            }
            else {
                my_log_file_path = log_file_path;
                return setFileStream(log_file_path);
            }
        }

        /***********************************************************
         *  Get the log file path where the log records are send to.
         ***********************************************************/
        const std::string getLogFilePath() const {
            std::lock_guard<std::mutex> lock(my_mutex);
            return my_log_file_path;
        }

    private:
        SyncQueue<LogRecord> log_queue;
        std::atomic<Level> my_level;
        std::string my_log_file_path;
        std::shared_ptr<std::ofstream> log_file_stream;
        std::shared_ptr<std::thread> write_thread;
        std::atomic<bool> need_to_stop;
        mutable std::mutex my_mutex;
        static std::shared_ptr<Logger> my_logger;

        /***********************************************************
         *  A private constructor. The parameter "level" specifies
         *  the lowest severity that will be dispatched to the log
         *  file. The parameter "log_file_path" specifies the log file
         *  path where the log records are send to.
         ***********************************************************/
        Logger(Level level, const std::string& log_file_path = "out.log")
        : my_level(level), log_queue(100), my_log_file_path(log_file_path), need_to_stop(false) {
            setFileStream(log_file_path);
            write_thread = std::make_shared<std::thread>([this] { runWriteThread(); } );
        }

        /***********************************************************
         *  Set file stream with the log file path.
         ***********************************************************/
        bool setFileStream(const std::string& log_file_path) {
            log_file_stream = std::make_shared<std::ofstream>();
            log_file_stream->open(log_file_path, std::ofstream::app);

            if (!log_file_stream->is_open()) {
                std::cerr << "Failed to open log file: " << log_file_path << std::endl;
                log_file_stream = nullptr;
                return false;
            }

            return true;
        }

        /***********************************************************
         *  Run the write thread until logger is stopped.
         *  Write every log record it takes from the log queue,
         *  waiting if the task queue is empty.
         ***********************************************************/
        void runWriteThread() {
            while (!need_to_stop) {
                std::string log;
                if (log_queue.take(log) && log_file_stream) {
                    *log_file_stream << log << std::endl;
                }
            }
        }

        /***********************************************************
         *  Convert a level to a string.
         ***********************************************************/
        const std::string levelToString(Level level) const {
            switch(level) {
                case Level::FATAL:
                    return "FATAL";
                case Level::ERROR:
                    return "ERROR";
                case Level::WARN:
                    return "WARN";
                case Level::INFO:
                    return "INFO";
                case Level::DEBUG:
                    return "DEBUG";
            }
        }

        /***********************************************************
         *  Get the current time as a string.
         ***********************************************************/
        const std::string getCurrentTime() const {
            auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::stringstream tStream;
            tStream << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
            return tStream.str();
        }
    };

    std::shared_ptr<Logger> Logger::my_logger = nullptr;

    /***********************************************************
    *  Log messages to the log file.
    ***********************************************************/
    #define LOG_FATAL(LOG_MESSAGE) Logger::getLogger()->write(acht::Logger::Level::FATAL, LOG_MESSAGE);
    #define LOG_ERROR(LOG_MESSAGE) Logger::getLogger()->write(acht::Logger::Level::ERROR, LOG_MESSAGE);
    #define LOG_WARN(LOG_MESSAGE) Logger::getLogger()->write(acht::Logger::Level::WARN, LOG_MESSAGE);
    #define LOG_INFO(LOG_MESSAGE) Logger::getLogger()->write(acht::Logger::Level::INFO, LOG_MESSAGE);
    #define LOG_DEBUG(LOG_MESSAGE) Logger::getLogger()->write(acht::Logger::Level::DEBUG, LOG_MESSAGE);
}

#endif