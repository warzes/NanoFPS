#include "Base.h"
#include "Core.h"

#pragma region Log

std::ostream& operator<<(std::ostream& os, const float2& i)
{
	os << "(" << i.x << ", " << i.y << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const float3& i)
{
	os << "(" << i.x << ", " << i.y << ", " << i.z << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const float4& i)
{
	os << "(" << i.r << ", " << i.g << ", " << i.b << ", " << i.a << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const uint3& i)
{
	os << "(" << i.x << ", " << i.y << ", " << i.z << ")";
	return os;
}

static Log sLogInstance;

Log::~Log()
{
	Shutdown();
}

bool Log::Initialize(uint32_t mode, const char* filePath, std::ostream* consoleStream)
{
	if (sLogInstance.m_modes != LOG_MODE_OFF) return false;

	bool result = sLogInstance.createObjects(mode, filePath, consoleStream);
	if (!result) return false;

	sLogInstance.Lock();
	{
		sLogInstance << "Logging started" << std::endl;
		sLogInstance.Flush(LOG_LEVEL_DEFAULT);
	}
	sLogInstance.Unlock();

	return true;
}

void Log::Shutdown()
{
	if (sLogInstance.m_modes == LOG_MODE_OFF) return;

	sLogInstance.Lock();
	{
		sLogInstance << "Logging stopped" << std::endl;
		sLogInstance.Flush(LOG_LEVEL_DEFAULT);

		sLogInstance.destroyObjects();
	}
	sLogInstance.Unlock();
}

Log* Log::Get()
{
	Log* ptr = nullptr;
#if !defined(DISABLE_AUTO_LOG)
	if (sLogInstance.m_modes == LOG_MODE_OFF)
	{
		bool res = Log::Initialize(LOG_MODE_CONSOLE | LOG_MODE_FILE, LOG_DEFAULT_PATH);
		if (!res) return nullptr;
	}
#endif
	if (sLogInstance.m_modes != LOG_MODE_OFF)
		ptr = &sLogInstance;

	return ptr;
}

bool Log::IsActive()
{
#if !defined(DISABLE_AUTO_LOG)
	Log::Get();
#endif
	bool result = (sLogInstance.m_modes != LOG_MODE_OFF);
	return result;
}

bool Log::IsModeActive(LogMode mode)
{
	bool result = ((sLogInstance.m_modes & mode) != 0);
	return result;
}

bool Log::createObjects(uint32_t modes, const char* filePath, std::ostream* consoleStream)
{
	m_modes = modes;
	if ((m_modes & LOG_MODE_FILE) != 0)
	{
		m_filePath = filePath;
		if (!m_filePath.empty())
		{
			m_fileStream.open(m_filePath.c_str());
		}
	}
	if ((m_modes & LOG_MODE_CONSOLE) != 0)
	{
		m_consoleStream = consoleStream;
	}
	return true;
}

void Log::destroyObjects()
{
	m_modes = LOG_MODE_OFF;
	m_filePath = "";
	if (m_fileStream.is_open())
	{
		m_fileStream.close();
	}
}

void Log::write(const char* msg, LogLevel level)
{
	std::string levelString;
	switch (level)
	{
	case LOG_LEVEL_WARN:
		levelString = "[WARNING] ";
		break;
	case LOG_LEVEL_DEBUG:
		levelString = "[DEBUG] ";
		break;
	case LOG_LEVEL_ERROR:
		levelString = "[ERROR] ";
		break;
	case LOG_LEVEL_FATAL:
		levelString = "[FATAL ERROR] ";
		break;
	case LOG_LEVEL_INFO:
	case LOG_LEVEL_DEFAULT:
	default:
		levelString = "";
		break;
	}

	// Console
	if ((m_modes & LOG_MODE_CONSOLE) != 0)
	{
#if defined(_MSC_VER)
		if (IsDebuggerPresent())
		{
			std::string debugMsg = levelString;
			debugMsg.append(msg);
			OutputDebugStringA(debugMsg.c_str());
		}
		else
		{
			*m_consoleStream << levelString << msg;
		}
#elif defined(_ANDROID)
		android_LogPriority prio;
		switch (level)
		{
		case LOG_LEVEL_WARN:
			prio = ANDROID_LOG_WARN;
			break;
		case LOG_LEVEL_DEBUG:
			prio = ANDROID_LOG_DEBUG;
			break;
		case LOG_LEVEL_ERROR:
			prio = ANDROID_LOG_ERROR;
			break;
		case LOG_LEVEL_FATAL:
			prio = ANDROID_LOG_FATAL;
			break;
		case LOG_LEVEL_INFO:
		case LOG_LEVEL_DEFAULT:
		default:
			prio = ANDROID_LOG_INFO;
			break;
		}
		__android_log_write(prio, "Nano", msg);
#else
		*m_consoleStream << levelString << msg;
#endif
	}
	// File
	if (((m_modes & LOG_MODE_FILE) != 0) && (m_fileStream.is_open()))
	{
		m_fileStream << levelString << msg;
	}
}

void Log::Lock()
{
	m_writeMutex.lock();
}

void Log::Unlock()
{
	m_writeMutex.unlock();
}

void Log::Flush(LogLevel level)
{
	// Write anything that's in the buffer
	if (m_buffer.str().size() > 0)
	{
		write(m_buffer.str().c_str(), level);
	}

	// Signal flush for console
	if ((m_modes & LOG_MODE_CONSOLE) != 0)
	{
#if defined(_MSC_VER)
		if (!IsDebuggerPresent())
		{
			*m_consoleStream << std::flush;
		}
#else
		*m_consoleStream << std::flush;
#endif
	}

	if (((m_modes & LOG_MODE_FILE) != 0) && (m_fileStream.is_open()))
		m_fileStream.flush();

	m_buffer.str(std::string());
	m_buffer.clear();
}

#pragma endregion
