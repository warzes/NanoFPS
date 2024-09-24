#pragma once

// TODO: сюда также загрузку шрифтов и мешей. А также все что грузится в данные и не завязано на рендер (шейдеры например в рендер... но возможно код шейдеров здесь)

#pragma region Bitmap

class Bitmap final
{
public:
	enum DataType
	{
		DATA_TYPE_UNDEFINED = 0,
		DATA_TYPE_UINT8,
		DATA_TYPE_UINT16,
		DATA_TYPE_UINT32,
		DATA_TYPE_FLOAT,
	};

	enum Format
	{
		FORMAT_UNDEFINED = 0,
		FORMAT_R_UINT8,
		FORMAT_RG_UINT8,
		FORMAT_RGB_UINT8,
		FORMAT_RGBA_UINT8,
		FORMAT_R_UINT16,
		FORMAT_RG_UINT16,
		FORMAT_RGB_UINT16,
		FORMAT_RGBA_UINT16,
		FORMAT_R_UINT32,
		FORMAT_RG_UINT32,
		FORMAT_RGB_UINT32,
		FORMAT_RGBA_UINT32,
		FORMAT_R_FLOAT,
		FORMAT_RG_FLOAT,
		FORMAT_RGB_FLOAT,
		FORMAT_RGBA_FLOAT,
	};

	Bitmap();
	Bitmap(const Bitmap& obj);
	~Bitmap();

	Bitmap& operator=(const Bitmap& rhs);

	// Creates a bitmap with internal storage.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, Bitmap* bitmap);
	// Creates a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Bitmap* bitmap);
	// Creates a bitmap with external storage.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, char* externalStorage, Bitmap* bitmap);
	// Returns a bitmap with internal storage.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, Result* result = nullptr);
	// Returns a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* externalStorage, Result* result = nullptr);
	// Returns a bitmap with external storage.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, char* externalStorage, Result* result = nullptr);

	// Returns true if dimensions are greater than one, format is valid, and storage is valid
	bool IsOk() const;

	uint32_t       GetWidth() const { return m_width; }
	uint32_t       GetHeight() const { return m_height; }
	Bitmap::Format GetFormat() const { return m_format; };
	uint32_t       GetChannelCount() const { return m_channelCount; }
	uint32_t       GetPixelStride() const { return m_pixelStride; }
	uint32_t       GetRowStride() const { return m_rowStride; }
	char*          GetData() const { return m_data; }
	uint64_t       GetFootprintSize(uint32_t rowStrideAlignment = 1) const;

	Result Resize(uint32_t width, uint32_t height);
	Result ScaleTo(Bitmap* pTargetBitmap) const;
	Result ScaleTo(Bitmap* pTargetBitmap, stbir_filter filterType) const;

	template <typename PixelDataType>
	void Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a);

	// Returns byte address of pixel at (x,y)
	char* GetPixelAddress(uint32_t x, uint32_t y);
	const char* GetPixelAddress(uint32_t x, uint32_t y) const;

	// These functions will return null if the Bitmap's format doesn't the function. For example, GetPixel8u() returns null if the format is FORMAT_RGBA_FLOAT.
	uint8_t* GetPixel8u(uint32_t x, uint32_t y);
	const uint8_t* GetPixel8u(uint32_t x, uint32_t y) const;
	uint16_t* GetPixel16u(uint32_t x, uint32_t y);
	const uint16_t* GetPixel16u(uint32_t x, uint32_t y) const;
	uint32_t* GetPixel32u(uint32_t x, uint32_t y);
	const uint32_t* GetPixel32u(uint32_t x, uint32_t y) const;
	float* GetPixel32f(uint32_t x, uint32_t y);
	const float* GetPixel32f(uint32_t x, uint32_t y) const;

	static uint32_t         ChannelSize(Bitmap::Format value);
	static uint32_t         ChannelCount(Bitmap::Format value);
	static Bitmap::DataType ChannelDataType(Bitmap::Format value);
	static uint32_t         FormatSize(Bitmap::Format value);
	static uint64_t         StorageFootprint(uint32_t width, uint32_t height, Bitmap::Format format);

	static Result GetFileProperties(const std::filesystem::path& path, uint32_t* pWidth, uint32_t* pHeight, Bitmap::Format* pFormat);
	static Result LoadFile(const std::filesystem::path& path, Bitmap* pBitmap);
	static Result SaveFilePNG(const std::filesystem::path& path, const Bitmap* pBitmap);
	static bool   IsBitmapFile(const std::filesystem::path& path);

	static Result LoadFromMemory(const size_t dataSize, const void* pData, Bitmap* pBitmap);

	class PixelIterator final
	{
		friend class Bitmap;
	public:
		void Reset()
		{
			m_x = 0;
			m_y = 0;
			m_pixelAddress = m_bitmap->GetPixelAddress(m_x, m_y);
		}

		bool Done() const
		{
			bool done = (m_y >= m_bitmap->GetHeight());
			return done;
		}

		bool Next()
		{
			if (Done()) return false;

			m_x += 1;
			m_pixelAddress += m_bitmap->GetPixelStride();
			if (m_x == m_bitmap->GetWidth())
			{
				m_y += 1;
				m_x = 0;
				m_pixelAddress = m_bitmap->GetPixelAddress(m_x, m_y);
			}

			return Done() ? false : true;
		}

		uint32_t       GetX() const { return m_x; }
		uint32_t       GetY() const { return m_y; }
		Bitmap::Format GetFormat() const { return m_bitmap->GetFormat(); }
		uint32_t       GetChannelCount() const { return Bitmap::ChannelCount(GetFormat()); }

		template <typename T>
		T* GetPixelAddress() const { return reinterpret_cast<T*>(m_pixelAddress); }

	private:
		PixelIterator(Bitmap* bitmap) : m_bitmap(bitmap)
		{
			Reset();
		}
		Bitmap*  m_bitmap = nullptr;
		uint32_t m_x = 0;
		uint32_t m_y = 0;
		char*    m_pixelAddress = nullptr;
	};

	PixelIterator GetPixelIterator() { return PixelIterator(this); }

private:
	void   InternalCtor();
	Result InternalInitialize(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage);
	Result InternalCopy(const Bitmap& obj);
	void   FreeStbiDataIfNeeded();

	// Stbi-specific functions/wrappers.
	// These arguments generally mirror those for stbi_load, except format which is used to determine whether the call should be made to stbi_load or stbi_loadf (that is, whether the file to be read is in integer or floating point format).
	static char* StbiLoad(const std::filesystem::path& path, Bitmap::Format format, int* pWidth, int* pHeight, int* pChannels, int desiredChannels);
	// These arugments mirror those for stbi_info.
	static Result StbiInfo(const std::filesystem::path& path, int* pX, int* pY, int* pComp);

	uint32_t          m_width = 0;
	uint32_t          m_height = 0;
	Bitmap::Format    m_format = Bitmap::FORMAT_UNDEFINED;
	uint32_t          m_channelCount = 0;
	uint32_t          m_pixelStride = 0;
	uint32_t          m_rowStride = 0;
	char*             m_data = nullptr;
	bool              m_dataIsFromStbi = false;
	std::vector<char> m_internalStorage = {};
};

template <typename PixelDataType>
void Bitmap::Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a)
{
	ASSERT_MSG(m_data != nullptr, "data is null");
	ASSERT_MSG(m_format != Bitmap::FORMAT_UNDEFINED, "format is undefined");

	PixelDataType rgba[4] = { r, g, b, a };

	const uint32_t channelCount = Bitmap::ChannelCount(m_format);

	char* pData = m_data;
	for (uint32_t y = 0; y < m_height; ++y)
	{
		for (uint32_t x = 0; x < m_width; ++x)
		{
			PixelDataType* pPixelData = reinterpret_cast<PixelDataType*>(pData);
			for (uint32_t c = 0; c < channelCount; ++c)
			{
				pPixelData[c] = rgba[c];
			}
			pData += m_pixelStride;
		}
	}
}

#pragma endregion
