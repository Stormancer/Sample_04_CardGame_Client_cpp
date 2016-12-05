#pragma once
#include "ILogger.h"

namespace Stormancer
{
	class Logger :public ILogger
	{
		// H�rit� via ILogger
		virtual void log(const std::string message) override;
		virtual void log(LogLevel level, const std::string category, const std::string message, const std::string data) override;
		virtual void log(const std::exception & e) override;
	};
}