#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include "SynchronousQueue.hpp"
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

		enum class Level {
			FATAL,
			ERROR,
			WARN,
			INFO,
			DEBUG
		};

		static std::shared_ptr<Logger> getLogger(Level level = Level::INFO, 
			const std::string& logFilePath = "acht_log.log") {
			if (myLogger == nullptr || myLogger->myLevel != level)
				myLogger = std::shared_ptr<Logger>(new Logger(level, logFilePath));
			return myLogger;
		}

		static void destroyLogger() {
			myLogger = nullptr;
		}

		~Logger() {
			stop();
			if (logFileStream) {
				logFileStream->close();
				logFileStream = nullptr;
			}
		}

		
		void write(Level level, const LogMessage& logMsg) {
			if (level > myLevel)
				return;

			// Create log record
			std::stringstream logRecordStream;
			logRecordStream << getCurrentTime()
				<< " [" << levelToString(level) << "] "
				<< logMsg;

			// Add the log record to log queue
			logQueue.put(logRecordStream.str());
		}


		void setLevel(Level level) {
			myLevel = level;
		}

		Level getLevel() const {
			return myLevel;
		}

		const std::string getLevelString() const {
			levelToString(myLevel);
		}


		void stop() {
			if (!needToStop) {
				needToStop = true;

				// Stop the log queue
				logQueue.stop();

				// Wait until all log records are written to file
				writeThread->join();
				writeThread = nullptr;
			}
		}

		void start() {
			if (needToStop) {
				needToStop = false;
				writeThread = std::make_shared<std::thread>([this] { runWriteThread(); } );
				logQueue.start();
			}
		}

		bool setLogFilePath(const std::string& logFilePath) {
			if (myLogFilePath == logFilePath)
				return true;
			return setFileStream(logFilePath);
		}


	private:
		SynchronousQueue<LogRecord> logQueue;
		std::atomic<Level> myLevel;
		std::string myLogFilePath;
		std::shared_ptr<std::ofstream> logFileStream;
		std::shared_ptr<std::thread> writeThread;
		std::atomic<bool> needToStop;
		static std::shared_ptr<Logger> myLogger;


		Logger(Level level, const std::string& logFilePath) 
		: myLevel(level), logQueue(100), myLogFilePath(logFilePath), needToStop(false) {
			setFileStream(logFilePath);
			writeThread = std::make_shared<std::thread>([this] { runWriteThread(); } );
		}

		bool setFileStream(const std::string& logFilePath) {
			logFileStream = std::make_shared<std::ofstream>();
			logFileStream->open(logFilePath, std::ofstream::app);

			if (!logFileStream->is_open()) {
				std::cerr << "Failed to open log file: " << logFilePath << std::endl; 
				logFileStream = nullptr;
				return false;
			}

			return true;
		}


		void runWriteThread() {
			while (!needToStop) {
				std::string log;
				if (logQueue.take(log) && logFileStream) {
					*logFileStream << std::unitbuf << log << std::endl;
				}
			}
		}

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

		std::string getCurrentTime() const {
			auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::stringstream tStream;
			tStream << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
			return tStream.str();
		}

	};

	std::shared_ptr<Logger> Logger::myLogger = nullptr;

}

#endif
