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

namespace acht {

	class Logger {


	public:
		enum class Level {
			FATAL,
			ERROR,
			WARN,
			INFO,
			DEBUG
		};

		static std::shared_ptr<Logger> getLogger(Level level = Level::INFO) {
			if (myLogger = nullptr)
				myLogger = std::shared_ptr<Logger>(new Logger(level));
			return myLogger;
		}

		// ~Logger() {
		// 	
		// }

		void setLevel(Level level) {
			myLevel = level;
		}

		Level getLevel() const {
			return myLevel;
		}

		const std::string getLevelString() const {
			switch(myLevel) {
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

	private:
		using LogRecord = std::string;
		using LogMessage = std::string;

		Level myLevel;
		SynchronousQueue<LogRecord> logQueue;
		std::shared_ptr<std::ofstream> logFileStream;
		std::shared_ptr<std::thread> writeThread;
		bool needToStop;
		static std::shared_ptr<Logger> myLogger;

		// const std::string levelString[5] = {
		// 	"FATAL",
		// 	"ERROR",
		// 	"WARN",
		// 	"INFO",
		// 	"DEBUG"
		// };

		Logger(Level level) : myLevel(level), logQueue(100), needToStop(false) {
			initializeFileStream();
			writeThread = std::make_shared<std::thread>([this] { run(); } );
		}

		void initializeFileStream() {
			std::string logFileName = "acht_log.log";

			logFileStream = std::make_shared<std::ofstream>();
			logFileStream->open(logFileName, std::ofstream::app);

			if (!logFileStream->is_open()) {
				std::cerr << "Failed to open log file: " << logFileName << std::endl; 
				logFileStream = nullptr;
			}
		}

		void write(Level level, const LogMessage& logMsg) {
			if (level > myLevel)
				return;

			// Create log record
			std::stringstream logRecordStream;
			logRecordStream << getCurrentTime()
				<< "[" << getLevelString() << "] "
				<< logMsg;

			// Add the log record to log queue
			logQueue.put(logRecordStream.str());
		}

		void run() {
			while (!needToStop) {
				std::string log;
				if (logQueue.take(log) && logFileStream) {
					*logFileStream << log << std::endl;
				}
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
