#pragma once

#pragma region Container

template <typename T>
typename std::vector<T>::iterator Find(std::vector<T>& container, const T& element)
{
	typename std::vector<T>::iterator it = std::find(
		std::begin(container),
		std::end(container),
		element);
	return it;
}

template <typename T>
typename std::vector<T>::const_iterator Find(const std::vector<T>& container, const T& element)
{
	typename std::vector<T>::iterator it = std::find(
		std::begin(container),
		std::end(container),
		element);
	return it;
}

template <typename T, class UnaryPredicate>
typename std::vector<T>::iterator FindIf(std::vector<T>& container, UnaryPredicate predicate)
{
	typename std::vector<T>::iterator it = std::find_if(
		std::begin(container),
		std::end(container),
		predicate);
	return it;
}

template <typename T, class UnaryPredicate>
typename std::vector<T>::const_iterator FindIf(const std::vector<T>& container, UnaryPredicate predicate)
{
	typename std::vector<T>::const_iterator it = std::find_if(
		std::begin(container),
		std::end(container),
		predicate);
	return it;
}

template <typename T>
bool ElementExists(const T& elem, const std::vector<T>& container)
{
	auto it = std::find(std::begin(container), std::end(container), elem);
	bool exists = (it != std::end(container));
	return exists;
}

template <typename T>
bool GetElement(uint32_t index, const std::vector<T>& container, T* pElem)
{
	if (!IsIndexInRange(index, container)) {
		return false;
	}
	*pElem = container[index];
	return true;
}

template <typename T>
void AppendElements(const std::vector<T>& elements, std::vector<T>& container)
{
	if (!elements.empty()) {
		std::copy(
			std::begin(elements),
			std::end(elements),
			std::back_inserter(container));
	}
}

template <typename T>
void RemoveElement(const T& elem, std::vector<T>& container)
{
	container.erase(
		std::remove(std::begin(container), std::end(container), elem),
		container.end());
}

template <typename T, class UnaryPredicate>
void RemoveElementIf(std::vector<T>& container, UnaryPredicate predicate)
{
	container.erase(
		std::remove_if(std::begin(container), std::end(container), predicate),
		container.end());
}

template <typename T>
void Unique(std::vector<T>& container)
{
	auto it = std::unique(std::begin(container), std::end(container));
	container.resize(std::distance(std::begin(container), it));
}

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
	bool Init();
	void Shutdown();
	void Mount(const std::string& newDir, const std::string& mountPoint, bool appendToPath = true);
	std::string Read(const std::string& filename); // TODO: return optional
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

	simdjson::dom::parser  m_parser;
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
	uint32_t       m_width = 0;
	uint32_t       m_height = 0;
	unsigned char* m_data = nullptr;
};

class BinaryParser
{
public:
	explicit BinaryParser(const std::string& filename);
	BinaryParser(const BinaryParser&) = delete;
	BinaryParser(BinaryParser&&) = delete;

	BinaryParser& operator=(const BinaryParser&) = delete;
	BinaryParser& operator=(BinaryParser&&) = delete;

protected:
	bool readU8(uint8_t& value) { return readValue(value); }
	bool readU16(uint16_t& value) { return readValue(value); }
	bool readFloat(float& value) { return readValue(value); }
	bool readVec3(glm::vec3& value) { return readValue(value); }
	// u8 length; char string[length];
	bool readShortString(std::string& string) { return readString<uint8_t>(string); }

	// u16 size; T vector[size];
	template<class T>
	bool readVector(std::vector<T>& vector)
	{
		return readVectorImpl<uint16_t>(vector);
	}

private:
	bool readBytes(size_t numBytes, void* output);

	template<class T>
	bool readValue(T& value)
	{
		return readBytes(sizeof(value), &value);
	}

	template<class LengthType>
	bool readString(std::string& string)
	{
		LengthType length;
		if (!readValue(length))
		{
			return false;
		}
		string.resize(length);
		return readBytes(length, string.data());
	}

	template<class LengthType, class ValueType>
	bool readVectorImpl(std::vector<ValueType>& vector)
	{
		LengthType size;
		if (!readValue(size))
		{
			return false;
		}
		vector.resize(size);
		return readBytes(size * sizeof(ValueType), vector.data());
	}

	std::string m_binary;
	const char* m_current;
	size_t      m_remainingBytes;
};

#pragma endregion
