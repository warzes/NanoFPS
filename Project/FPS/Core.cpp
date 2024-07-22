#include "Base.h"
#include "Core.h"

#pragma region Clock

Time Clock::GetElapsedTime() const
{
	if (IsRunning())
		return std::chrono::duration_cast<std::chrono::microseconds>(ClockImpl::now() - m_refPoint);
	return std::chrono::duration_cast<std::chrono::microseconds>(m_stopPoint - m_refPoint);
}

bool Clock::IsRunning() const
{
	return m_stopPoint == ClockImpl::time_point();
}

void Clock::Start()
{
	if (!IsRunning())
	{
		m_refPoint += ClockImpl::now() - m_stopPoint;
		m_stopPoint = {};
	}
}

void Clock::Stop()
{
	if (IsRunning())
		m_stopPoint = ClockImpl::now();
}

Time Clock::Restart()
{
	const Time elapsed = GetElapsedTime();
	m_refPoint = ClockImpl::now();
	m_stopPoint = {};
	return elapsed;
}

inline Time Clock::Reset()
{
	const Time elapsed = GetElapsedTime();
	m_refPoint = ClockImpl::now();
	m_stopPoint = m_refPoint;
	return elapsed;
}

#pragma endregion

#pragma region Log

extern void AppEnd();

void Print(const std::string& text)
{
	puts(text.c_str());
}

void Warning(const std::string& text)
{
	Print("WARNING: " + text);
}

void Error(const std::string& text)
{
	Print("ERROR: " + text);
}

void Fatal(const std::string& text)
{
	Print("FATAL: " + text);
	AppEnd();
}
#pragma endregion

#pragma region IO

std::optional<std::string> LoadTextFile(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		Error("File '" + path.string() + "' does not exist.");
		return std::nullopt;
	}

	const char whitespace = ' ';
	const std::string includeIdentifier = "#include "; // TODO: проверять что в include используются кавычки или нет
	const std::string rootFolder = std::filesystem::absolute(path).parent_path().string() + "/";

	std::string shaderCode = "";
	std::ifstream file(path);
	if (!file.is_open())
	{
		Error("Failed to load shader file " + path.string());
		return std::nullopt;
	}

	std::string lineBuffer;
	while (std::getline(file, lineBuffer))
	{
		if (lineBuffer.find(includeIdentifier) != lineBuffer.npos)
		{
			std::size_t index = lineBuffer.find_last_of(whitespace);
			std::string includeFullPath = rootFolder + lineBuffer.substr(index + 1);

			auto subtext = LoadTextFile(includeFullPath.c_str());
			if (!subtext) return std::nullopt;

			shaderCode += *subtext;
		}
		else
		{
			shaderCode += lineBuffer + '\n';
		}
	}

	file.close();

	return shaderCode;
}

#pragma endregion
