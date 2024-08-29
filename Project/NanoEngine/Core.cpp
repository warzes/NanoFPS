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
		* m_consoleStream << std::flush;
#endif
	}

	if (((m_modes & LOG_MODE_FILE) != 0) && (m_fileStream.is_open()))
		m_fileStream.flush();

	m_buffer.str(std::string());
	m_buffer.clear();
}

#pragma endregion

#pragma region String Utils

namespace string_util
{
	// -------------------------------------------------------------------------------------------------
	// Misc
	// -------------------------------------------------------------------------------------------------

	std::string ToLowerCopy(const std::string& s)
	{
		std::string s2 = s;
		std::transform(
			s2.cbegin(),
			s2.cend(),
			s2.begin(),
			[](std::string::value_type c) { return std::tolower(c); });
		return s2;
	}

	std::string ToUpperCopy(const std::string& s)
	{
		std::string s2 = s;
		std::transform(
			s2.cbegin(),
			s2.cend(),
			s2.begin(),
			[](std::string::value_type c) { return std::toupper(c); });
		return s2;
	}

	void TrimLeft(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
	}

	void TrimRight(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(),
				s.end());
	}

	std::string TrimCopy(const std::string& s)
	{
		std::string sc = s;
		TrimLeft(sc);
		TrimRight(sc);
		return sc;
	}

	std::string_view TrimBothEnds(std::string_view s, std::string_view c)
	{
		if (s.size() == 0)
			return s;

		auto strBegin = s.find_first_not_of(c);
		auto strEnd = s.find_last_not_of(c);
		auto strRange = strEnd - strBegin + 1;
		return s.substr(strBegin, strRange);
	}

	std::pair<std::string_view, std::string_view> SplitInTwo(std::string_view s, char delimiter)
	{
		std::pair<std::string_view, std::string_view> stringViewPair;
		if (s.size() == 0)
			return stringViewPair;


		size_t delimiterIndex = s.find(delimiter);
		if (delimiterIndex == std::string_view::npos)
		{
			stringViewPair.first = s;
			return stringViewPair;
		}

		stringViewPair.first = s.substr(0, delimiterIndex);
		stringViewPair.second = s.substr(delimiterIndex + 1);
		return stringViewPair;
	}

	// -------------------------------------------------------------------------------------------------
	// Formatting Strings
	// -------------------------------------------------------------------------------------------------

	std::string WrapText(const std::string& s, size_t width, size_t indent)
	{
		if (indent >= width)
			return s;

		auto remainingString = TrimCopy(s);

		size_t      textWidth = width - indent;
		std::string wrappedText = "";
		while (remainingString != "")
		{
			// Section off the next line from the remaining string, format it, and append it to wrappedText
			size_t lineLength = remainingString.length();
			if (lineLength > textWidth)
			{
				// Break the line at textWidth
				lineLength = textWidth;
				// If the character after the line break is not empty, the line break will interrupt a word
				// so try to insert the line break at the last empty space on this line
				if (!std::isspace(remainingString[textWidth]))
				{
					lineLength = remainingString.find_last_of(" \t", textWidth);
					// In case there is no empty space on this entire line, go back to breaking the word at textWidth
					if (lineLength == std::string::npos)
					{
						lineLength = textWidth;
					}
				}
			}
			std::string newLine = remainingString.substr(0, lineLength);
			TrimRight(newLine);
			wrappedText += std::string(indent, ' ') + newLine + "\n";

			// Update the remaining string
			remainingString = remainingString.substr(lineLength);
			TrimLeft(remainingString);
		}
		return wrappedText;
	}

} // namespace string_util

// -------------------------------------------------------------------------------------------------
// Parsing Strings
// -------------------------------------------------------------------------------------------------

Result string_util::Parse(std::string_view valueStr, std::string& parsedValue)
{
	parsedValue = std::string(valueStr);
	return SUCCESS;
}

Result string_util::Parse(std::string_view valueStr, bool& parsedValue)
{
	if (valueStr == "")
	{
		parsedValue = true;
		return SUCCESS;
	}
	std::stringstream ss{ std::string(valueStr) };
	bool              valueAsBool;
	ss >> valueAsBool;
	if (ss.fail())
	{
		ss.clear();
		ss >> std::boolalpha >> valueAsBool;
		if (ss.fail())
		{
			LOG_ERROR("could not be parsed as bool: " << valueStr);
			return ERROR_FAILED;
		}
	}
	parsedValue = valueAsBool;
	return SUCCESS;
}

Result string_util::Parse(std::string_view valueStr, std::pair<int, int>& parsedValues)
{
	if (std::count(valueStr.cbegin(), valueStr.cend(), 'x') != 1) {
		LOG_ERROR("invalid number of 'x', resolution string must be in format <Width>x<Height>: " << valueStr);
		return ERROR_FAILED;
	}
	std::pair<std::string_view, std::string_view> parseResolution = SplitInTwo(valueStr, 'x');
	if (parseResolution.first.length() == 0 || parseResolution.second.length() == 0)
	{
		LOG_ERROR("both width and height must be defined, resolution string must be in format <Width>x<Height>: " << valueStr);
		return ERROR_FAILED;
	}
	int  N, M;
	auto res = Parse(parseResolution.first, N);
	if (Failed(res))
	{
		LOG_ERROR("width cannot be parsed");
		return res;
	}
	res = Parse(parseResolution.second, M);
	if (Failed(res))
	{
		LOG_ERROR("height cannot be parsed");
		return res;
	}
	parsedValues.first = N;
	parsedValues.second = M;
	return SUCCESS;
}

#pragma endregion

#pragma region CpuInfo

const char* GetX86LongMicroarchitectureName(cpu_features::X86Microarchitecture march)
{
	switch (march)
	{
		default: break;
		case cpu_features::INTEL_CORE     : return "Core"; break;
		case cpu_features::INTEL_PNR      : return "Penryn"; break;
		case cpu_features::INTEL_NHM      : return "Nehalem"; break;
		case cpu_features::INTEL_ATOM_BNL : return "Bonnell"; break;
		case cpu_features::INTEL_WSM      : return "Westmere"; break;
		case cpu_features::INTEL_SNB      : return "Sandybridge"; break;
		case cpu_features::INTEL_IVB      : return "Ivybridge"; break;
		case cpu_features::INTEL_ATOM_SMT : return "Silvermont"; break;
		case cpu_features::INTEL_HSW      : return "Haswell"; break;
		case cpu_features::INTEL_BDW      : return "Broadwell"; break;
		case cpu_features::INTEL_SKL      : return "Skylake"; break;
		case cpu_features::INTEL_ATOM_GMT : return "Goldmont"; break;
		case cpu_features::INTEL_KBL      : return "Kaby Lake"; break;
		case cpu_features::INTEL_CFL      : return "Coffee Lake"; break;
		case cpu_features::INTEL_WHL      : return "Whiskey Lake"; break;
		case cpu_features::INTEL_CNL      : return "Cannon Lake"; break;
		case cpu_features::INTEL_ICL      : return "Ice Lake"; break;
		case cpu_features::INTEL_TGL      : return "Tiger Lake"; break;
		case cpu_features::INTEL_SPR      : return "Sapphire Rapids"; break;
		case cpu_features::AMD_HAMMER     : return "K8"; break;
		case cpu_features::AMD_K10        : return "K10"; break;
		case cpu_features::AMD_BOBCAT     : return "K14"; break;
		case cpu_features::AMD_BULLDOZER  : return "K15"; break;
		case cpu_features::AMD_JAGUAR     : return "K16"; break;
		case cpu_features::AMD_ZEN        : return "K17"; break;
	};
	// TODO: остальные
	return "Unknown X86 Architecture";
}

CpuInfo getX86CpuInfo()
{
	cpu_features::X86Info              info = cpu_features::GetX86Info();
	cpu_features::X86Microarchitecture march = cpu_features::GetX86Microarchitecture(&info);

	CpuInfo cpuInfo                   = {};
	cpuInfo.m_brandString             = string_util::TrimCopy(info.brand_string);
	cpuInfo.m_vendorString            = string_util::TrimCopy(info.vendor);
	cpuInfo.m_microarchitectureString = string_util::TrimCopy(GetX86LongMicroarchitectureName(march));

	cpuInfo.m_features.sse                 = static_cast<bool>(info.features.sse);
	cpuInfo.m_features.sse2                = static_cast<bool>(info.features.sse2);
	cpuInfo.m_features.sse3                = static_cast<bool>(info.features.sse3);
	cpuInfo.m_features.ssse3               = static_cast<bool>(info.features.ssse3);
	cpuInfo.m_features.sse4_1              = static_cast<bool>(info.features.sse4_1);
	cpuInfo.m_features.sse4_2              = static_cast<bool>(info.features.sse4_2);
	cpuInfo.m_features.sse4a               = static_cast<bool>(info.features.sse4a);
	cpuInfo.m_features.avx                 = static_cast<bool>(info.features.avx);
	cpuInfo.m_features.avx2                = static_cast<bool>(info.features.avx2);
	cpuInfo.m_features.avx512f             = static_cast<bool>(info.features.avx512f);
	cpuInfo.m_features.avx512cd            = static_cast<bool>(info.features.avx512cd);
	cpuInfo.m_features.avx512er            = static_cast<bool>(info.features.avx512er);
	cpuInfo.m_features.avx512pf            = static_cast<bool>(info.features.avx512pf);
	cpuInfo.m_features.avx512bw            = static_cast<bool>(info.features.avx512bw);
	cpuInfo.m_features.avx512dq            = static_cast<bool>(info.features.avx512dq);
	cpuInfo.m_features.avx512vl            = static_cast<bool>(info.features.avx512vl);
	cpuInfo.m_features.avx512ifma          = static_cast<bool>(info.features.avx512ifma);
	cpuInfo.m_features.avx512vbmi          = static_cast<bool>(info.features.avx512vbmi);
	cpuInfo.m_features.avx512vbmi2         = static_cast<bool>(info.features.avx512vbmi2);
	cpuInfo.m_features.avx512vnni          = static_cast<bool>(info.features.avx512vnni);
	cpuInfo.m_features.avx512bitalg        = static_cast<bool>(info.features.avx512bitalg);
	cpuInfo.m_features.avx512vpopcntdq     = static_cast<bool>(info.features.avx512vpopcntdq);
	cpuInfo.m_features.avx512_4vnniw       = static_cast<bool>(info.features.avx512_4vnniw);
	cpuInfo.m_features.avx512_4vbmi2       = static_cast<bool>(info.features.avx512_4vbmi2);
	cpuInfo.m_features.avx512_second_fma   = static_cast<bool>(info.features.avx512_second_fma);
	cpuInfo.m_features.avx512_4fmaps       = static_cast<bool>(info.features.avx512_4fmaps);
	cpuInfo.m_features.avx512_bf16         = static_cast<bool>(info.features.avx512_bf16);
	cpuInfo.m_features.avx512_vp2intersect = static_cast<bool>(info.features.avx512_vp2intersect);
	cpuInfo.m_features.amx_bf16            = static_cast<bool>(info.features.amx_bf16);
	cpuInfo.m_features.amx_tile            = static_cast<bool>(info.features.amx_tile);
	cpuInfo.m_features.amx_int8            = static_cast<bool>(info.features.amx_int8);

	return cpuInfo;
}

#pragma endregion

#pragma region Platform Info

static Platform sPlatform = Platform();

Platform::Platform()
{
	m_cpuInfo = getX86CpuInfo();
}

PlatformId Platform::GetPlatformId()
{
#if defined(_LINUX)
	return PLATFORM_ID_LINUX;
#elif defined(_WIN32)
	return PLATFORM_ID_MSW;
#else
	return PLATFORM_ID_UNDEFINED;
#endif
}

const char* Platform::GetPlatformString()
{
#if defined(_LINUX)
	return "Linux";
#elif defined(_WIN32)
	return "Windows";
#elif defined(_ANDROID)
	return "Android";
#endif
	return "<unknown platform>";
}

const CpuInfo& Platform::GetCpuInfo()
{
	return sPlatform.m_cpuInfo;
}

#pragma endregion

#pragma region Timer

#if defined(_WIN32)
static double sNanosPerCount = 0.0;
#endif

#if defined(_WIN32)
static TimerResult Win32SleepNanos(double nanos)
{
	const uint64_t k_min_sleep_threshold = 2 * TIMER_MILLIS_TO_NANOS;

	uint64_t    now = 0;
	TimerResult result = Timer::Timestamp(&now);
	if (result != TIMER_RESULT_SUCCESS)
	{
		return result;
	}

	uint64_t target = now + (uint64_t)nanos;

	for (;;) {
		result = Timer::Timestamp(&now);
		if (result != TIMER_RESULT_SUCCESS)
		{
			return result;
		}

		if (now >= target)
		{
			break;
		}

		uint64_t diff = target - now;
		if (diff >= k_min_sleep_threshold)
		{
			DWORD millis = (DWORD)(diff * TIMER_NANOS_TO_MILLIS);
			Sleep(millis);
		}
		else
		{
			Sleep(0);
		}
	}

	return TIMER_RESULT_SUCCESS;
}
#endif

#if defined(_LINUX) || defined(_ANDROID)
static TimerResult SleepSeconds(double seconds)
{
	double nanos = seconds * (double)TIMER_SECONDS_TO_NANOS;
	double secs = floor(nanos * (double)TIMER_NANOS_TO_SECONDS);

	struct timespec req;
	req.tv_sec = (time_t)secs;
	req.tv_nsec = (long)(nanos - (secs * (double)TIMER_SECONDS_TO_NANOS));

	int result = nanosleep(&req, NULL);
	assert(result == 0);
	if (result != 0)
	{
		return TIMER_RESULT_ERROR_SLEEP_FAILED;
	}

	return TIMER_RESULT_SUCCESS;
}
#elif defined(_WIN32)
static TimerResult SleepSeconds(double seconds)
{
	double nanos = seconds * TIMER_SECONDS_TO_NANOS;
	Win32SleepNanos(nanos);
	return TIMER_RESULT_SUCCESS;
}
#endif

#if defined(_LINUX) || defined(_ANDROID)
TimerResult SleepMillis(double millis)
{
	double nanos = millis * (double)TIMER_MILLIS_TO_NANOS;
	double secs = floor(nanos * (double)TIMER_NANOS_TO_SECONDS);

	struct timespec req;
	req.tv_sec = (time_t)secs;
	req.tv_nsec = (long)(nanos - (secs * (double)TIMER_SECONDS_TO_NANOS));

	int result = nanosleep(&req, NULL);
	assert(result == 0);
	if (result != 0)
	{
		return TIMER_RESULT_ERROR_SLEEP_FAILED;
	}

	return TIMER_RESULT_SUCCESS;
}
#elif defined(_WIN32)
static TimerResult SleepMillis(double millis)
{
	double nanos = millis * (double)TIMER_MILLIS_TO_NANOS;
	Win32SleepNanos(nanos);
	return TIMER_RESULT_SUCCESS;
}
#endif

#if defined(_LINUX) || defined(_ANDROID)
static TimerResult SleepMicros(double micros)
{
	double nanos = micros * (double)TIMER_MICROS_TO_NANOS;
	double secs = floor(nanos * (double)TIMER_NANOS_TO_SECONDS);

	struct timespec req;
	req.tv_sec = (time_t)secs;
	req.tv_nsec = (long)(nanos - (secs * (double)TIMER_SECONDS_TO_NANOS));

	int result = nanosleep(&req, NULL);
	assert(result == 0);
	if (result != 0)
	{
		return TIMER_RESULT_ERROR_SLEEP_FAILED;
	}

	return TIMER_RESULT_SUCCESS;
}
#elif defined(_WIN32)
static TimerResult SleepMicros(double micros)
{
	double nanos = micros * (double)TIMER_MICROS_TO_NANOS;
	Win32SleepNanos(nanos);
	return TIMER_RESULT_SUCCESS;
}
#endif

#if defined(_LINUX) || defined(_ANDROID)
static TimerResult SleepNanos(double nanos)
{
	double secs = floor(nanos * (double)TIMER_NANOS_TO_SECONDS);

	struct timespec req;
	req.tv_sec = (time_t)secs;
	req.tv_nsec = (long)(nanos - (secs * (double)TIMER_SECONDS_TO_NANOS));

	int result = nanosleep(&req, NULL);
	assert(result == 0);
	if (result != 0)
	{
		return TIMER_RESULT_ERROR_SLEEP_FAILED;
	}

	return TIMER_RESULT_SUCCESS;
}
#elif defined(_WIN32)
static TimerResult SleepNanos(double nanos)
{
	Win32SleepNanos(nanos);
	return TIMER_RESULT_SUCCESS;
}
#endif

#if defined(_LINUX) || defined(_ANDROID)
TimerResult Timer::InitializeStaticData()
{
	return TIMER_RESULT_SUCCESS;
}
#elif defined(_WIN32)
TimerResult Timer::InitializeStaticData()
{
	LARGE_INTEGER frequency;
	BOOL          result = QueryPerformanceFrequency(&frequency);
	assert(result != FALSE);
	if (result == FALSE)
	{
		return TIMER_RESULT_ERROR_TIMESTAMP_FAILED;
	}
	sNanosPerCount = 1.0e9 / (double)frequency.QuadPart;
	return TIMER_RESULT_SUCCESS;
}
#endif

#if defined(_LINUX) || defined(_ANDROID)
TimerResult Timer::Timestamp(uint64_t* pTimestamp)
{
	assert(pTimestamp != NULL);
#if !defined(TIMER_DISABLE_NULL_POINTER_CHECK)
	if (pTimestamp == NULL)
	{
		return TIMER_RESULT_ERROR_NULL_POINTER;
	}
#endif

	// Read
	struct timespec tp;
	int             result = clock_gettime(TIMER_CLK_ID, &tp);
	assert(result == 0);
	if (result != 0)
	{
		return TIMER_RESULT_ERROR_TIMESTAMP_FAILED;
	}

	// Convert seconds to nanoseconds
	uint64_t timestamp = (uint64_t)tp.tv_sec * (uint64_t)TIMER_SECONDS_TO_NANOS;
	// Add nanoseconds
	timestamp += (uint64_t)tp.tv_nsec;

	*pTimestamp = timestamp;

	return TIMER_RESULT_SUCCESS;
}
#elif defined(_WIN32)
TimerResult Timer::Timestamp(uint64_t* pTimestamp)
{
	assert(pTimestamp != NULL);
#if !defined(TIMER_DISABLE_NULL_POINTER_CHECK)
	if (pTimestamp == NULL)
	{
		return TIMER_RESULT_ERROR_NULL_POINTER;
	}
#endif

	// Read
	LARGE_INTEGER counter;
	BOOL          result = QueryPerformanceCounter(&counter);
	assert(result != FALSE);
	if (result == FALSE)
	{
		return TIMER_RESULT_ERROR_TIMESTAMP_FAILED;
	}

	//
	// QPC: https://msdn.microsoft.com/en-us/library/ms644904(v=VS.85).aspx
	//
	// According to the QPC link above, QueryPerformanceCounter returns a
	// timestamp that's < 1us.
	//
	double timestamp = (double)counter.QuadPart * sNanosPerCount;

	*pTimestamp = static_cast<uint64_t>(timestamp);

	return TIMER_RESULT_SUCCESS;
}
#endif

double Timer::TimestampToSeconds(uint64_t timestamp)
{
	double seconds = (double)timestamp * (double)TIMER_NANOS_TO_SECONDS;
	return seconds;
}

double Timer::TimestampToMillis(uint64_t timestamp)
{
	double millis = (double)timestamp * (double)TIMER_NANOS_TO_MILLIS;
	return millis;
}

double Timer::TimestampToMicros(uint64_t timestamp)
{
	double micros = (double)timestamp * (double)TIMER_NANOS_TO_MICROS;
	return micros;
}

double Timer::TimestampToNanos(uint64_t timestamp)
{
	double nanos = (double)timestamp;
	return nanos;
}

TimerResult Timer::SleepSeconds(double seconds)
{
	return SleepSeconds(seconds);
}

TimerResult Timer::SleepMillis(double millis)
{
	return SleepMillis(millis);
}

TimerResult Timer::SleepMicros(double micros)
{
	return SleepMicros(micros);
}

TimerResult Timer::SleepNanos(double nanos)
{
	return SleepNanos(nanos);
}

TimerResult Timer::Start()
{
	TimerResult result = Timestamp(&m_startTimestamp);
	return result;
}

TimerResult Timer::Stop()
{
	TimerResult result = Timestamp(&m_stopTimestamp);
	return result;
}

uint64_t Timer::StartTimestamp() const
{
	return m_startTimestamp;
}

uint64_t Timer::StopTimestamp() const
{
	return m_stopTimestamp;
}

double Timer::SecondsSinceStart() const
{
	uint64_t diff = 0;
	if (m_startTimestamp > 0)
	{
		if (m_stopTimestamp > 0)
		{
			diff = m_stopTimestamp - m_startTimestamp;
		}
		else
		{
			uint64_t now = 0;
			Timestamp(&now);
			diff = now - m_startTimestamp;
		}
	}
	double seconds = TimestampToSeconds(diff);
	return seconds;
}

double Timer::MillisSinceStart() const
{
	uint64_t diff = 0;
	if (m_startTimestamp > 0)
	{
		if (m_stopTimestamp > 0)
		{
			diff = m_stopTimestamp - m_startTimestamp;
		}
		else {
			uint64_t now = 0;
			Timestamp(&now);
			diff = now - m_startTimestamp;
		}
	}
	double millis = TimestampToMillis(diff);
	return millis;
}

double Timer::MicrosSinceStart() const
{
	uint64_t diff = 0;
	if (m_startTimestamp > 0)
	{
		if (m_stopTimestamp > 0)
		{
			diff = m_stopTimestamp - m_startTimestamp;
		}
		else
		{
			uint64_t now = 0;
			Timestamp(&now);
			diff = now - m_startTimestamp;
		}
	}
	double micros = TimestampToMicros(diff);
	return micros;
}

double Timer::NanosSinceStart() const
{
	uint64_t diff = 0;
	if (m_startTimestamp > 0)
	{
		if (m_stopTimestamp > 0)
		{
			diff = m_stopTimestamp - m_startTimestamp;
		}
		else {
			uint64_t now = 0;
			Timestamp(&now);
			diff = now - m_startTimestamp;
		}
	}
	double nanos = TimestampToMicros(diff);
	return nanos;
}

ScopedTimer::ScopedTimer(const std::string& message) : m_message(message)
{
	ASSERT_MSG(m_timer.Start() == TIMER_RESULT_SUCCESS, "Timer start failed.");
}

ScopedTimer::~ScopedTimer()
{
	ASSERT_MSG(m_timer.Stop() == TIMER_RESULT_SUCCESS, "Timer stop failed.");
	const float elapsed = static_cast<float>(m_timer.SecondsSinceStart());
	LOG_INFO(m_message + ": " + FloatString(elapsed) << " seconds.");
}

#pragma endregion

#pragma region File System

namespace fs
{
#if defined(_ANDROID)
	void set_android_context(android_app* androidContext)
	{
		gAndroidContext = androidContext;
	}
#endif

	File::File() : m_handleType(BAD_HANDLE)
	{
	}

	File::~File()
	{
		switch (m_handleType)
		{
		case ASSET_HANDLE:
#if defined(_ANDROID)
			AAsset_close(mAsset);
#else
			ASSERT_MSG(false, "Bad implem. This case should never be reached.");
#endif
			break;
		case STREAM_HANDLE:
			m_stream.close();
			break;
		default:
			break;
		}
	}

	bool File::Open(const std::filesystem::path& path)
	{
#if defined(_ANDROID)
		if (!path.is_absolute())
		{
			mAsset = AAssetManager_open(gAndroidContext->activity->assetManager, path.c_str(), AASSET_MODE_BUFFER);
			mHandleType = mAsset != nullptr ? ASSET_HANDLE : BAD_HANDLE;
			mFileSize = mHandleType == ASSET_HANDLE ? AAsset_getLength(mAsset) : 0;
			mFileOffset = 0;
			mBuffer = AAsset_getBuffer(mAsset);
			return mHandleType == ASSET_HANDLE;
		}
#endif

		m_stream.open(path, std::ios::binary);
		if (!m_stream.good())
		{
			return false;
		}

		m_stream.seekg(0, std::ios::end);
		m_fileSize = m_stream.tellg();
		m_stream.seekg(0, std::ios::beg);
		m_fileOffset = 0;
		m_handleType = STREAM_HANDLE;
		return true;
	}

	bool File::IsValid() const
	{
		if (m_handleType == STREAM_HANDLE)
		{
			return m_stream.good();
		}
		return m_handleType == ASSET_HANDLE && m_asset != nullptr;
	}

	bool File::IsMapped() const
	{
		return IsValid() && m_buffer != nullptr;
	}

	size_t File::Read(void* buffer, size_t count)
	{
		ASSERT_MSG(IsValid(), "Calling File::read() on an invalid file.");

		size_t readCount = 0;

		if (IsMapped())
		{
			readCount = std::min(count, GetLength() - m_fileOffset);
			memcpy(buffer, reinterpret_cast<const uint8_t*>(GetMappedData()) + m_fileOffset, readCount);
		}
		else if (m_handleType == STREAM_HANDLE)
		{
			m_stream.read(reinterpret_cast<char*>(buffer), count);
			readCount = m_stream.gcount();
		}
#if defined(_ANDROID)
		else if (mHandleType == ASSET_HANDLE)
		{
			readCount = AAsset_read(mAsset, buffer, count);
		}
#endif

		m_fileOffset += readCount;
		return readCount;
	}

	size_t File::GetLength() const
	{
		return m_fileSize;
	}

	const void* File::GetMappedData() const
	{
		ASSERT_MSG(IsMapped(), "Called GetMappedData() on an non-mapped file.");
		return m_buffer;
	}

	bool FileStream::Open(const char* path)
	{
		auto optional_buffer = load_file(path);
		if (!optional_buffer.has_value())
			return false;
		m_buffer = optional_buffer.value();
		setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + m_buffer.size());
		return true;
	}

	std::optional<std::vector<char>> load_file(const std::filesystem::path& path)
	{
		fs::File file;
		if (!file.Open(path))
		{
			return std::nullopt;
		}
		const size_t      size = file.GetLength();
		std::vector<char> buffer(size);
		const size_t      readSize = file.Read(buffer.data(), size);
		if (readSize != size) {
			return std::nullopt;
		}
		return buffer;
	}

	bool path_exists(const std::filesystem::path& path)
	{
#if defined(_ANDROID)
		if (!path.is_absolute())
		{
			AAsset* temp_file = AAssetManager_open(gAndroidContext->activity->assetManager, path.c_str(), AASSET_MODE_BUFFER);
			if (temp_file != nullptr)
			{
				AAsset_close(temp_file);
				return true;
			}

			// So it's not a file. Check if it's a subdirectory
			AAssetDir* temp_dir = AAssetManager_openDir(gAndroidContext->activity->assetManager, path.c_str());
			if (temp_dir != nullptr)
			{
				AAssetDir_close(temp_dir);
				return true;
			}
			return false;
		}
#endif

		return std::filesystem::exists(path);
	}

#if defined(_ANDROID)
	std::filesystem::path GetInternalDataPath()
	{
		return std::filesystem::path(gAndroidContext->activity->internalDataPath);
	}

	std::filesystem::path GetExternalDataPath()
	{
		return std::filesystem::path(gAndroidContext->activity->externalDataPath);
	}
#endif

	std::filesystem::path GetDefaultOutputDirectory()
	{
#if defined(_ANDROID)
		return fs::GetExternalDataPath();
#else
		return std::filesystem::current_path();
#endif
	}

	std::filesystem::path GetFullPath(const std::filesystem::path& partialPath, const std::filesystem::path& defaultFolder, const std::string& regexToReplace, const std::string& replaceString)
	{
		ASSERT_MSG(partialPath.string() != "", "Partial path cannot be empty string");
		ASSERT_MSG(partialPath.has_filename(), "Partial path cannot be a folder");

		std::filesystem::path fullPath = partialPath;

		if (regexToReplace != "")
		{
			std::string fileName = fullPath.filename().string();
			std::string newfileName = std::regex_replace(fileName, std::regex(regexToReplace), replaceString);
			fullPath.replace_filename(std::filesystem::path(newfileName));
		}

		if (!fullPath.has_root_path())
		{
			fullPath = defaultFolder / fullPath;
		}

		return fullPath;
	}

} // namespace fs

#pragma endregion

#pragma region Bounding Volume

AABB::AABB(const OBB& obb)
{
	Set(obb);
}

AABB& AABB::operator=(const OBB& rhs)
{
	Set(rhs);
	return *this;
}

void AABB::Set(const OBB& obb)
{
	float3 obbVertices[8];
	obb.GetPoints(obbVertices);

	Set(obbVertices[0]);
	for (size_t i = 1; i < 8; ++i) 
	{
		Expand(obbVertices[i]);
	}
}

void AABB::Transform(const float4x4& matrix, float3 obbVertices[8]) const
{
	obbVertices[0] = matrix * float4(m_min.x, m_max.y, m_min.z, 1.0f);
	obbVertices[1] = matrix * float4(m_min.x, m_min.y, m_min.z, 1.0f);
	obbVertices[2] = matrix * float4(m_max.x, m_min.y, m_min.z, 1.0f);
	obbVertices[3] = matrix * float4(m_max.x, m_max.y, m_min.z, 1.0f);
	obbVertices[4] = matrix * float4(m_min.x, m_max.y, m_max.z, 1.0f);
	obbVertices[5] = matrix * float4(m_min.x, m_min.y, m_max.z, 1.0f);
	obbVertices[6] = matrix * float4(m_max.x, m_min.y, m_max.z, 1.0f);
	obbVertices[7] = matrix * float4(m_max.x, m_max.y, m_max.z, 1.0f);
}

OBB::OBB(const AABB& aabb)
{
	Set(aabb);
}

void OBB::Set(const AABB& aabb)
{
	m_center = aabb.GetCenter();
	m_size = aabb.GetSize();
	m_u = aabb.GetU();
	m_v = aabb.GetV();
	m_w = aabb.GetW();
}

void OBB::GetPoints(float3 obbVertices[8]) const
{
	float3 s = m_size / 2.0f;
	float3 u = s.x * m_u;
	float3 v = s.y * m_v;
	float3 w = s.z * m_w;
	obbVertices[0] = m_center - u + v - w;
	obbVertices[1] = m_center - u - v - w;
	obbVertices[2] = m_center + u - v - w;
	obbVertices[3] = m_center + w + v - w;
	obbVertices[4] = m_center - u + v + w;
	obbVertices[5] = m_center - u - v + w;
	obbVertices[6] = m_center + u - v + w;
	obbVertices[7] = m_center + w + v + w;
}

#pragma endregion

#pragma region Transform

Transform::Transform(const float3& translation)
{
	SetTranslation(translation);
}

void Transform::SetTranslation(const float3& value)
{
	m_translation = value;
	m_dirty.translation = true;
	m_dirty.concatenated = true;
}

void Transform::SetTranslation(float x, float y, float z)
{
	SetTranslation(float3(x, y, z));
}

void Transform::SetRotation(const float3& value)
{
	m_rotation = value;
	m_dirty.rotation = true;
	m_dirty.concatenated = true;
}

void Transform::SetRotation(float x, float y, float z)
{
	SetRotation(float3(x, y, z));
}

void Transform::SetScale(const float3& value)
{
	m_scale = value;
	m_dirty.scale = true;
}

void Transform::SetScale(float x, float y, float z)
{
	SetScale(float3(x, y, z));
}

void Transform::SetRotationOrder(Transform::RotationOrder value)
{
	m_rotationOrder = value;
	m_dirty.rotation = true;
	m_dirty.concatenated = true;
}

const float4x4& Transform::GetTranslationMatrix() const
{
	if (m_dirty.translation) {
		m_translationMatrix = glm::translate(m_translation);
		m_dirty.translation = false;
		m_dirty.concatenated = true;
	}
	return m_translationMatrix;
}

const float4x4& Transform::GetRotationMatrix() const
{
	if (m_dirty.rotation)
	{
		float4x4 xm = glm::rotate(m_rotation.x, float3(1, 0, 0));
		float4x4 ym = glm::rotate(m_rotation.y, float3(0, 1, 0));
		float4x4 zm = glm::rotate(m_rotation.z, float3(0, 0, 1));
		switch (m_rotationOrder)
		{
		case RotationOrder::XYZ: m_rotationMatrix = xm * ym * zm; break;
		case RotationOrder::XZY: m_rotationMatrix = xm * zm * ym; break;
		case RotationOrder::YZX: m_rotationMatrix = ym * zm * xm; break;
		case RotationOrder::YXZ: m_rotationMatrix = ym * xm * zm; break;
		case RotationOrder::ZXY: m_rotationMatrix = zm * xm * ym; break;
		case RotationOrder::ZYX: m_rotationMatrix = zm * ym * xm; break;
		}
		m_dirty.rotation = false;
		m_dirty.concatenated = true;
	}
	return m_rotationMatrix;
}

const float4x4& Transform::GetScaleMatrix() const
{
	if (m_dirty.scale)
	{
		m_scaleMatrix = glm::scale(m_scale);
		m_dirty.scale = false;
		m_dirty.concatenated = true;
	}
	return m_scaleMatrix;
}

const float4x4& Transform::GetConcatenatedMatrix() const
{
	if (m_dirty.concatenated)
	{
		const float4x4& T = GetTranslationMatrix();
		const float4x4& R = GetRotationMatrix();
		const float4x4& S = GetScaleMatrix();
		// Matrices are column-major in GLM, so we do not need to reverse the order.
		m_concatenatedMatrix = T * R * S;
		m_dirty.concatenated = false;
	}
	return m_concatenatedMatrix;
}

#pragma endregion

#pragma region Camera

Camera::Camera(bool pixelAligned)
	: mPixelAligned(pixelAligned)
{
	LookAt(CAMERA_DEFAULT_EYE_POSITION, CAMERA_DEFAULT_LOOK_AT, CAMERA_DEFAULT_WORLD_UP);
}

Camera::Camera(float nearClip, float farClip, bool pixelAligned)
	: mPixelAligned(pixelAligned),
	mNearClip(nearClip),
	mFarClip(farClip)
{
}

void Camera::LookAt(const float3& eye, const float3& target, const float3& up)
{
	const float3 yAxis = mPixelAligned ? float3(1, -1, 1) : float3(1, 1, 1);
	mEyePosition = eye;
	mTarget = target;
	mWorldUp = up;
	mViewDirection = glm::normalize(mTarget - mEyePosition);
	mViewMatrix = glm::scale(yAxis) * glm::lookAt(mEyePosition, mTarget, mWorldUp);
	mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
	mInverseViewMatrix = glm::inverse(mViewMatrix);
}

float3 Camera::WorldToViewPoint(const float3& worldPoint) const
{
	float3 viewPoint = float3(mViewMatrix * float4(worldPoint, 1.0f));
	return viewPoint;
}

float3 Camera::WorldToViewVector(const float3& worldVector) const
{
	float3 viewPoint = float3(mViewMatrix * float4(worldVector, 0.0f));
	return viewPoint;
}

void Camera::MoveAlongViewDirection(float distance)
{
	float3 eyePosition = mEyePosition + (distance * mViewDirection);
	LookAt(eyePosition, mTarget, mWorldUp);
}

PerspCamera::PerspCamera()
{
}

PerspCamera::PerspCamera(
	float horizFovDegrees,
	float aspect,
	float nearClip,
	float farClip)
	: Camera(nearClip, farClip)
{
	SetPerspective(
		horizFovDegrees,
		aspect,
		nearClip,
		farClip);
}

PerspCamera::PerspCamera(
	const float3& eye,
	const float3& target,
	const float3& up,
	float         horizFovDegrees,
	float         aspect,
	float         nearClip,
	float         farClip)
	: Camera(nearClip, farClip)
{
	LookAt(eye, target, up);

	SetPerspective(
		horizFovDegrees,
		aspect,
		nearClip,
		farClip);
}

PerspCamera::PerspCamera(
	uint32_t pixelWidth,
	uint32_t pixelHeight,
	float    horizFovDegrees)
	: Camera(true)
{
	float aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
	float eyeX = pixelWidth / 2.0f;
	float eyeY = pixelHeight / 2.0f;
	float halfFov = atan(tan(glm::radians(horizFovDegrees) / 2.0f) / aspect); // horiz fov -> vert fov
	float theTan = tanf(halfFov);
	float dist = eyeY / theTan;
	float nearClip = dist / 10.0f;
	float farClip = dist * 10.0f;

	SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
	LookAt(float3(eyeX, eyeY, dist), float3(eyeX, eyeY, 0.0f));
}

PerspCamera::PerspCamera(
	uint32_t pixelWidth,
	uint32_t pixelHeight,
	float    horizFovDegrees,
	float    nearClip,
	float    farClip)
	: Camera(nearClip, farClip, true)
{
	float aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
	float eyeX = pixelWidth / 2.0f;
	float eyeY = pixelHeight / 2.0f;
	float halfFov = atan(tan(glm::radians(horizFovDegrees) / 2.0f) / aspect); // horiz fov -> vert fov
	float theTan = tanf(halfFov);
	float dist = eyeY / theTan;

	SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
	LookAt(float3(eyeX, eyeY, dist), float3(eyeX, eyeY, 0.0f));
}

PerspCamera::~PerspCamera()
{
}

void PerspCamera::SetPerspective(
	float horizFovDegrees,
	float aspect,
	float nearClip,
	float farClip)
{
	mHorizFovDegrees = horizFovDegrees;
	mAspect = aspect;
	mNearClip = nearClip;
	mFarClip = farClip;

	float horizFovRadians = glm::radians(mHorizFovDegrees);
	float vertFovRadians = 2.0f * atan(tan(horizFovRadians / 2.0f) / mAspect);
	mVertFovDegrees = glm::degrees(vertFovRadians);

	mProjectionMatrix = glm::perspective(
		vertFovRadians,
		mAspect,
		mNearClip,
		mFarClip);

	mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;
}

void PerspCamera::FitToBoundingBox(const float3& bboxMinWorldSpace, const float3& bbxoMaxWorldSpace)
{
	float3   min = bboxMinWorldSpace;
	float3   max = bbxoMaxWorldSpace;
	float3   target = (min + max) / 2.0f;
	float3   up = glm::normalize(mInverseViewMatrix * float4(0, 1, 0, 0));
	float4x4 viewSpaceMatrix = glm::lookAt(mEyePosition, target, up);

	// World space oriented bounding box
	float3 obb[8] = {
		{min.x, max.y, min.z},
		{min.x, min.y, min.z},
		{max.x, min.y, min.z},
		{max.x, max.y, min.z},
		{min.x, max.y, max.z},
		{min.x, min.y, max.z},
		{max.x, min.y, max.z},
		{max.x, max.y, max.z},
	};

	// Tranform obb from world space to view space
	for (uint32_t i = 0; i < 8; ++i) {
		obb[i] = viewSpaceMatrix * float4(obb[i], 1.0f);
	}

	// Get aabb from obb in view space
	min = max = obb[0];
	for (uint32_t i = 1; i < 8; ++i) {
		min = glm::min(min, obb[i]);
		max = glm::max(max, obb[i]);
	}

	// Get x,y extent max
	float xmax = glm::max(glm::abs(min.x), glm::abs(max.x));
	float ymax = glm::max(glm::abs(min.y), glm::abs(max.y));
	float rad = glm::max(xmax, ymax);
	float fov = mAspect < 1.0f ? mHorizFovDegrees : mVertFovDegrees;

	// Calculate distance
	float dist = rad / tan(glm::radians(fov / 2.0f));

	// Calculate eye position
	float3 dir = glm::normalize(mEyePosition - target);
	float3 eye = target + (dist + mNearClip) * dir;

	// Adjust camera look at
	LookAt(eye, target, up);
}

OrthoCamera::OrthoCamera()
{
}

OrthoCamera::OrthoCamera(
	float left,
	float right,
	float bottom,
	float top,
	float near_clip,
	float far_clip)
{
	SetOrthographic(
		left,
		right,
		bottom,
		top,
		near_clip,
		far_clip);
}

OrthoCamera::~OrthoCamera()
{
}

void OrthoCamera::SetOrthographic(
	float left,
	float right,
	float bottom,
	float top,
	float nearClip,
	float farClip)
{
	mLeft = left;
	mRight = right;
	mBottom = bottom;
	mTop = top;
	mNearClip = nearClip;
	mFarClip = farClip;

	mProjectionMatrix = glm::ortho(
		mLeft,
		mRight,
		mBottom,
		mTop,
		mNearClip,
		mFarClip);
}

ArcballCamera::ArcballCamera()
{
}

ArcballCamera::ArcballCamera(
	float horizFovDegrees,
	float aspect,
	float nearClip,
	float farClip)
	: PerspCamera(horizFovDegrees, aspect, nearClip, farClip)
{
}

ArcballCamera::ArcballCamera(
	const float3& eye,
	const float3& target,
	const float3& up,
	float         horizFovDegrees,
	float         aspect,
	float         nearClip,
	float         farClip)
	: PerspCamera(eye, target, up, horizFovDegrees, aspect, nearClip, farClip)
{
}

void ArcballCamera::UpdateCamera()
{
	mViewMatrix = mTranslationMatrix * glm::mat4_cast(mRotationQuat) * mCenterTranslationMatrix;
	mInverseViewMatrix = glm::inverse(mViewMatrix);
	mViewProjectionMatrix = mProjectionMatrix * mViewMatrix;

	// Transform the view space origin into world space for eye position
	mEyePosition = mInverseViewMatrix * float4(0, 0, 0, 1);
}

void ArcballCamera::LookAt(const float3& eye, const float3& target, const float3& up)
{
	Camera::LookAt(eye, target, up);

	float3 viewDir = target - eye;
	float3 zAxis = glm::normalize(viewDir);
	float3 xAxis = glm::normalize(glm::cross(zAxis, glm::normalize(up)));
	float3 yAxis = glm::normalize(glm::cross(xAxis, zAxis));
	xAxis = glm::normalize(glm::cross(zAxis, yAxis));

	mCenterTranslationMatrix = glm::inverse(glm::translate(target));
	mTranslationMatrix = glm::translate(float3(0.0f, 0.0f, -glm::length(viewDir)));
	mRotationQuat = glm::normalize(glm::quat_cast(glm::transpose(glm::mat3(xAxis, yAxis, -zAxis))));

	UpdateCamera();
}

static quat ScreenToArcball(const float2& p)
{
	float dist = glm::dot(p, p);

	// If we're on/in the sphere return the point on it
	if (dist <= 1.0f) {
		return glm::quat(0.0f, p.x, p.y, glm::sqrt(1.0f - dist));
	}

	// Otherwise we project the point onto the sphere
	const glm::vec2 proj = glm::normalize(p);
	return glm::quat(0.0f, proj.x, proj.y, 0.0f);
}

void ArcballCamera::Rotate(const float2& prevPos, const float2& curPos)
{
	const float2 kNormalizeDeviceCoordinatesMin = float2(-1, -1);
	const float2 kNormalizeDeviceCoordinatesMax = float2(1, 1);

	// Clamp mouse positions to stay in NDC
	float2 clampedCurPos = glm::clamp(curPos, kNormalizeDeviceCoordinatesMin, kNormalizeDeviceCoordinatesMax);
	float2 clampedPrevPos = glm::clamp(prevPos, kNormalizeDeviceCoordinatesMin, kNormalizeDeviceCoordinatesMax);

	quat mouseCurBall = ScreenToArcball(clampedCurPos);
	quat mousePrevBall = ScreenToArcball(clampedPrevPos);

	mRotationQuat = mouseCurBall * mousePrevBall * mRotationQuat;

	UpdateCamera();
}

void ArcballCamera::Pan(const float2& delta)
{
	float  zoomAmount = glm::abs(mTranslationMatrix[3][2]);
	float4 motion = float4(delta.x * zoomAmount, delta.y * zoomAmount, 0.0f, 0.0f);

	// Find the panning amount in the world space
	motion = mInverseViewMatrix * motion;

	mCenterTranslationMatrix = glm::translate(float3(motion)) * mCenterTranslationMatrix;

	UpdateCamera();
}

void ArcballCamera::Zoom(float amount)
{
	float3 motion = float3(0.0f, 0.0f, amount);

	mTranslationMatrix = glm::translate(motion) * mTranslationMatrix;

	UpdateCamera();
}

#pragma endregion