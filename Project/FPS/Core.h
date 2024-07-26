#pragma once

#pragma region Container

class CopyableByteSpan final : public std::span<const std::byte>
{
public:
	template<typename T> requires std::is_trivially_copyable_v<T>
	CopyableByteSpan(const T& t) : std::span<const std::byte>(std::as_bytes(std::span{ &t, static_cast<size_t>(1) })) {}

	template<typename T> requires std::is_trivially_copyable_v<T>
	CopyableByteSpan(std::span<const T> t) : std::span<const std::byte>(std::as_bytes(t)) {}

	template<typename T> requires std::is_trivially_copyable_v<T>
	CopyableByteSpan(std::span<T> t) : std::span<const std::byte>(std::as_bytes(t)) {}
};

#pragma endregion

#pragma region Time

class Time final
{
public:
	constexpr Time() = default;
	template <typename Rep, typename Period>
	constexpr Time(const std::chrono::duration<Rep, Period>& duration);

	[[nodiscard]] constexpr float AsSeconds() const;
	[[nodiscard]] constexpr int32_t AsMilliseconds() const;
	[[nodiscard]] constexpr int64_t AsMicroseconds() const;

	[[nodiscard]] constexpr std::chrono::microseconds ToDuration() const;
	template <typename Rep, typename Period>
	constexpr operator std::chrono::duration<Rep, Period>() const;

private:
	std::chrono::microseconds m_microseconds{};
};

#pragma region inline Time 

template<typename Rep, typename Period>
inline constexpr Time::Time(const std::chrono::duration<Rep, Period>& duration) : m_microseconds(duration)
{
}

inline constexpr float Time::AsSeconds() const
{
	return std::chrono::duration<float>(m_microseconds).count();
}

inline constexpr int32_t Time::AsMilliseconds() const
{
	return std::chrono::duration_cast<std::chrono::duration<std::int32_t, std::milli>>(m_microseconds).count();
}

inline constexpr int64_t Time::AsMicroseconds() const
{
	return m_microseconds.count();
}

inline constexpr std::chrono::microseconds Time::ToDuration() const
{
	return m_microseconds;
}

template<typename Rep, typename Period>
inline constexpr Time::operator std::chrono::duration<Rep, Period>() const
{
	return m_microseconds;
}

#pragma endregion

class Clock final
{
public:
	[[nodiscard]] Time GetElapsedTime() const;

	[[nodiscard]] bool IsRunning() const;

	void Start();
	void Stop();
	Time Restart();
	Time Reset();
private:
	using ClockImpl = std::conditional_t<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>;

	ClockImpl::time_point m_refPoint{ ClockImpl::now() };
	ClockImpl::time_point m_stopPoint;
};

#pragma endregion

#pragma region Log

namespace EngineApp
{
	extern void Exit();
}

void Print(const std::string& text);
void Warning(const std::string& text);
void Error(const std::string& text);
void Fatal(const std::string& text);

#pragma endregion

#pragma region IO

[[nodiscard]] std::optional<std::string> LoadTextFile(const std::filesystem::path& path);

namespace FileSystem
{
	void Init();
	void Shutdown();
	void Mount(const std::string& newDir, const std::string& mountPoint, bool appendToPath = true);
	std::string Read(const std::string& filename);
} // namespace FileSystem

class JsonFile final
{
public:
	explicit JsonFile(const std::string& filename);
	JsonFile(const JsonFile&) = delete;
	JsonFile(JsonFile&&) = delete;

	JsonFile& operator=(const JsonFile&) = delete;
	JsonFile& operator=(JsonFile&&) = delete;

	std::string GetString(const std::string& key);
	std::string GetString(const std::string& key, const std::string& fallback);

	int GetInteger(const std::string& key);
	int GetInteger(const std::string& key, int fallback);

	bool GetBoolean(const std::string& key);
	bool GetBoolean(const std::string& key, bool fallback);

private:
	template<class T>
	T getCriticalField(const std::string& key);

	template<class T>
	T getField(const std::string& key, const T& fallback);

	simdjson::dom::parser m_parser;
	simdjson::dom::element m_document;
};

class ImageFile final
{
public:
	ImageFile() = default;
	explicit ImageFile(const std::string& filename);
	ImageFile(const ImageFile&) = delete;
	ImageFile(ImageFile&& other) noexcept { Swap(other); }
	~ImageFile() { Release(); }

	ImageFile& operator=(const ImageFile&) = delete;
	ImageFile& operator=(ImageFile&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(ImageFile& other) noexcept;

	[[nodiscard]] uint32_t GetWidth() const { return m_width; }
	[[nodiscard]] uint32_t GetHeight() const { return m_height; }
	[[nodiscard]] const unsigned char* GetData() const { return m_data; }

private:
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	unsigned char* m_data = nullptr;
};

#pragma endregion
