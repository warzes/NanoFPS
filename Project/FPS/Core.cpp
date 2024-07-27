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
	EngineApp::Exit();
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

std::string GetLastError()
{
	return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

bool FileSystem::Init()
{
	if (!PHYSFS_init(__argv[0]))
	{
		Fatal("Failed to init PhysFS: " + GetLastError());
		return false;
	}
	return true;
}

void FileSystem::Shutdown()
{
	if (!PHYSFS_deinit())
		Fatal("Failed to shutdown PhysFS: " + GetLastError());
}

void FileSystem::Mount(const std::string& newDir, const std::string& mountPoint, bool appendToPath)
{
	if (!PHYSFS_mount(newDir.c_str(), mountPoint.c_str(), appendToPath))
	{
		Fatal("PhysFS failed to mount " + newDir + " at " + mountPoint + ": " + GetLastError());
	}
	else
	{
		Print("PhysFS mount " + newDir + " at " + mountPoint);
	}
}

std::string FileSystem::Read(const std::string& filename)
{
	PHYSFS_File* file = PHYSFS_openRead(filename.c_str());
	if (!file)
	{
		Fatal("PhysFS failed to open file " + filename + ": " + GetLastError());
		return {};
	}
	const PHYSFS_sint64 length = PHYSFS_fileLength(file);
	if (length == -1)
	{
		Fatal("PhysFS failed to get file size " + filename + ": " + GetLastError());
		return {};
	}
	std::string bytes(length, '\0');
	if (PHYSFS_readBytes(file, bytes.data(), length) == -1)
	{
		Fatal("PhysFS failed to read file " + filename + ": " + GetLastError());
	}
	return bytes;
}

JsonFile::JsonFile(const std::string& filename)
{
	const simdjson::error_code result = m_parser.parse(FileSystem::Read(filename)).get(m_document);
	if (result != simdjson::SUCCESS)
		Fatal("Failed to load json document: " + std::string(simdjson::error_message(result)));
}

template<class T>
T JsonFile::getCriticalField(const std::string& key)
{
	T value{};
	const simdjson::error_code result = m_document.at_key(key).get(value);
	if (result != simdjson::SUCCESS)
		Fatal("Failed to find key '" + key + "': " + std::string(simdjson::error_message(result)));
	return value;
}

template<class T>
T JsonFile::getField(const std::string& key, const T& fallback)
{
	T value{};
	const simdjson::error_code error = m_document.at_key(key).get(value);
	return error == simdjson::SUCCESS ? value : fallback;
}

std::string JsonFile::GetString(const std::string& key)
{
	return std::string(getCriticalField<std::string_view>(key));
}

std::string JsonFile::GetString(const std::string& key, const std::string& fallback)
{
	return std::string(getField<std::string_view>(key, fallback));
}

int JsonFile::GetInteger(const std::string& key)
{
	return static_cast<int>(getCriticalField<double>(key));
}

int JsonFile::GetInteger(const std::string& key, int fallback) {
	return static_cast<int>(getField<double>(key, fallback));
}

bool JsonFile::GetBoolean(const std::string& key)
{
	return getCriticalField<bool>(key);
}

bool JsonFile::GetBoolean(const std::string& key, bool fallback)
{
	return getField(key, fallback);
}

ImageFile::ImageFile(const std::string& filename)
{
	stbi_set_flip_vertically_on_load(true);

	std::string bytes = FileSystem::Read(filename);
	int         width = 0, height = 0;
	m_data = stbi_load_from_memory(
		reinterpret_cast<const stbi_uc*>(bytes.data()),
		static_cast<int>(bytes.length()),
		&width,
		&height,
		nullptr,
		STBI_rgb_alpha
	);
	m_width = width;
	m_height = height;
}

void ImageFile::Release()
{
	stbi_image_free(m_data);

	m_width = 0;
	m_height = 0;
	m_data = nullptr;
}

void ImageFile::Swap(ImageFile& other) noexcept
{
	std::swap(m_width, other.m_width);
	std::swap(m_height, other.m_height);
	std::swap(m_data, other.m_data);
}

BinaryParser::BinaryParser(const std::string& filename)
	: m_binary(FileSystem::Read(filename))
	, m_current(m_binary.data())
	, m_remainingBytes(m_binary.size()) {
}

bool BinaryParser::ReadBytes(size_t numBytes, void* output) {
	if (m_remainingBytes < numBytes) {
		Fatal("Not enough data in the binary file.");
		return false;
	}
	memcpy(output, m_current, numBytes);
	m_current += numBytes;
	m_remainingBytes -= numBytes;
	return true;
}


#pragma endregion
