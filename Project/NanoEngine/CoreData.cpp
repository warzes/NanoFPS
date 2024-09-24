#include "stdafx.h"
#include "Core.h"
#include "CoreData.h"

#pragma region Bitmap

static const char* kRadianceSig = "#?RADIANCE";
static const size_t kRadianceSigSize = 10;

Bitmap::Bitmap()
{
	InternalCtor();
}

Bitmap::Bitmap(const Bitmap& obj)
{
	Result ppxres = InternalCopy(obj);
	if (Failed(ppxres)) {
		InternalCtor();
		ASSERT_MSG(false, "copy failed");
	}
}

Bitmap::~Bitmap()
{
	FreeStbiDataIfNeeded();
}

Bitmap& Bitmap::operator=(const Bitmap& rhs)
{
	if (&rhs != this) {
		Result ppxres = InternalCopy(rhs);
		if (Failed(ppxres)) {
			InternalCtor();
			ASSERT_MSG(false, "copy failed");
		}
	}
	return *this;
}

void Bitmap::FreeStbiDataIfNeeded()
{
	if (m_dataIsFromStbi && !IsNull(m_data)) {
		stbi_image_free(m_data);
		m_data = nullptr;
		m_dataIsFromStbi = false;
	}
}

void Bitmap::InternalCtor()
{
	m_width = 0;
	m_height = 0;
	m_format = Bitmap::FORMAT_UNDEFINED;
	m_channelCount = 0;
	m_pixelStride = 0;
	m_rowStride = 0;
	m_data = nullptr;
	m_internalStorage.clear();
}

Result Bitmap::InternalInitialize(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage)
{
	if (format == Bitmap::FORMAT_UNDEFINED) {
		return ERROR_IMAGE_INVALID_FORMAT;
	}

	// In case of initialization to a preexisting object.
	FreeStbiDataIfNeeded();

	uint32_t minimumRowStride = (width * Bitmap::FormatSize(format));
	if ((rowStride > 0) && (rowStride < minimumRowStride)) {
		return ERROR_BITMAP_FOOTPRINT_MISMATCH;
	}
	if (rowStride == 0) {
		rowStride = minimumRowStride;
	}

	m_width = width;
	m_height = height;
	m_format = format;
	m_channelCount = Bitmap::ChannelCount(format);
	m_pixelStride = Bitmap::FormatSize(format);
	m_rowStride = rowStride;
	m_data = pExternalStorage;

	if (IsNull(m_data)) {
		size_t n = Bitmap::StorageFootprint(width, height, format);
		if (n > 0) {
			m_internalStorage.resize(n);
			if (m_internalStorage.size() != n) {
				return ERROR_ALLOCATION_FAILED;
			}

			m_data = m_internalStorage.data();
		}
	}

	return SUCCESS;
}

Result Bitmap::InternalCopy(const Bitmap& obj)
{
	// In case of copies into a preexisting object.
	FreeStbiDataIfNeeded();

	// Copy properties
	m_width = obj.m_width;
	m_height = obj.m_height;
	m_format = obj.m_format;
	m_channelCount = obj.m_channelCount;
	m_pixelStride = obj.m_pixelStride;
	m_rowStride = obj.m_rowStride;

	// Allocate storage
	size_t footprint = Bitmap::StorageFootprint(m_width, m_height, m_format);
	m_internalStorage.resize(footprint);
	if (m_internalStorage.size() != footprint) {
		return ERROR_ALLOCATION_FAILED;
	}
	m_data = m_internalStorage.data();

	uint64_t srcFootprint = obj.GetFootprintSize();
	if (srcFootprint != static_cast<uint64_t>(footprint)) {
		return ERROR_BITMAP_FOOTPRINT_MISMATCH;
	}

	memcpy(m_data, obj.m_data, footprint);

	return SUCCESS;
}

Result Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, Bitmap* pBitmap)
{
	ASSERT_NULL_ARG(pBitmap);
	if (IsNull(pBitmap)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	Result ppxres = pBitmap->InternalInitialize(width, height, format, 0, nullptr);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Bitmap* pBitmap)
{
	ASSERT_NULL_ARG(pBitmap);
	if (IsNull(pBitmap)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	Result ppxres = pBitmap->InternalInitialize(width, height, format, rowStride, pExternalStorage);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Bitmap* pBitmap)
{
	return Bitmap::Create(width, height, format, 0, pExternalStorage, pBitmap);
}

Bitmap Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, Result* pResult)
{
	Bitmap bitmap;
	Result ppxres = Bitmap::Create(width, height, format, &bitmap);
	if (Failed(ppxres)) {
		bitmap.InternalCtor();
	}
	if (!IsNull(pResult)) {
		*pResult = ppxres;
	}
	return bitmap;
}

Bitmap Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Result* pResult)
{
	Bitmap bitmap;
	Result ppxres = Bitmap::Create(width, height, format, rowStride, pExternalStorage, &bitmap);
	if (Failed(ppxres)) {
		bitmap.InternalCtor();
	}
	if (!IsNull(pResult)) {
		*pResult = ppxres;
	}
	return bitmap;
}

Bitmap Bitmap::Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Result* pResult)
{
	return Bitmap::Create(width, height, format, 0, pExternalStorage, pResult);
}

bool Bitmap::IsOk() const
{
	bool isSizeValid = (m_width > 0) && (m_height > 0);
	bool isFormatValid = (m_format != Bitmap::FORMAT_UNDEFINED);
	bool isStorageValid = (m_data != nullptr);
	return isSizeValid && isFormatValid && isStorageValid;
}

uint64_t Bitmap::GetFootprintSize(uint32_t rowStrideAlignment) const
{
	uint32_t alignedRowStride = RoundUp<uint32_t>(m_rowStride, rowStrideAlignment);
	uint64_t size = alignedRowStride * m_height;
	return size;
}

Result Bitmap::Resize(uint32_t width, uint32_t height)
{
	// If internal storage is empty then this bitmap is using external storage...so don't resize!
	if (m_internalStorage.empty()) {
		return ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE;
	}

	m_width = width;
	m_height = height;
	m_rowStride = width * m_pixelStride;

	size_t n = Bitmap::StorageFootprint(m_width, m_height, m_format);
	m_internalStorage.resize(n);
	m_data = (m_internalStorage.size() > 0) ? m_internalStorage.data() : nullptr;

	return SUCCESS;
}

Result Bitmap::ScaleTo(Bitmap* pTargetBitmap) const
{
	return ScaleTo(pTargetBitmap, STBIR_FILTER_DEFAULT);
}

Result Bitmap::ScaleTo(Bitmap* pTargetBitmap, stbir_filter filterType) const
{
	if (IsNull(pTargetBitmap)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	// Format must match
	if (pTargetBitmap->GetFormat() != m_format) {
		return ERROR_IMAGE_INVALID_FORMAT;
	}

	stbir_datatype datatype = InvalidValue<stbir_datatype>();
	switch (ChannelDataType(GetFormat())) {
	default: break;
	case Bitmap::DATA_TYPE_UINT8: datatype = STBIR_TYPE_UINT8; break;
	case Bitmap::DATA_TYPE_UINT16: datatype = STBIR_TYPE_UINT16; break;
	case Bitmap::DATA_TYPE_UINT32: datatype = STBIR_TYPE_UINT32; break;
	case Bitmap::DATA_TYPE_FLOAT: datatype = STBIR_TYPE_FLOAT; break;
	}

	int res = stbir_resize(
		static_cast<const void*>(GetData()),
		static_cast<int>(GetWidth()),
		static_cast<int>(GetHeight()),
		static_cast<int>(GetRowStride()),
		static_cast<void*>(pTargetBitmap->GetData()),
		static_cast<int>(pTargetBitmap->GetWidth()),
		static_cast<int>(pTargetBitmap->GetHeight()),
		static_cast<int>(pTargetBitmap->GetRowStride()),
		datatype,
		static_cast<int>(Bitmap::ChannelCount(GetFormat())),
		-1,
		0,
		STBIR_EDGE_CLAMP,
		STBIR_EDGE_CLAMP,
		filterType,
		filterType,
		STBIR_COLORSPACE_LINEAR,
		nullptr);

	if (res == 0) {
		return ERROR_IMAGE_RESIZE_FAILED;
	}

	return SUCCESS;
}

char* Bitmap::GetPixelAddress(uint32_t x, uint32_t y)
{
	char* pPixel = nullptr;
	if (!IsNull(m_data) && (x >= 0) && (x < m_width) && (y >= 0) && (y < m_height)) {
		size_t offset = (y * m_rowStride) + (x * m_pixelStride);
		pPixel = m_data + offset;
	}
	return pPixel;
}

const char* Bitmap::GetPixelAddress(uint32_t x, uint32_t y) const
{
	const char* pPixel = nullptr;
	if (!IsNull(m_data) && (x >= 0) && (x < m_width) && (y >= 0) && (y < m_height)) {
		size_t offset = (y * m_rowStride) + (x * m_pixelStride);
		pPixel = m_data + offset;
	}
	return pPixel;
}

uint8_t* Bitmap::GetPixel8u(uint32_t x, uint32_t y)
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_UINT8) {
		return nullptr;
	}
	return reinterpret_cast<uint8_t*>(GetPixelAddress(x, y));
}

const uint8_t* Bitmap::GetPixel8u(uint32_t x, uint32_t y) const
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_UINT8) {
		return nullptr;
	}
	return reinterpret_cast<const uint8_t*>(GetPixelAddress(x, y));
}

uint16_t* Bitmap::GetPixel16u(uint32_t x, uint32_t y)
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_UINT16) {
		return nullptr;
	}
	return reinterpret_cast<uint16_t*>(GetPixelAddress(x, y));
}

const uint16_t* Bitmap::GetPixel16u(uint32_t x, uint32_t y) const
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_UINT16) {
		return nullptr;
	}
	return reinterpret_cast<const uint16_t*>(GetPixelAddress(x, y));
}

uint32_t* Bitmap::GetPixel32u(uint32_t x, uint32_t y)
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_UINT32) {
		return nullptr;
	}
	return reinterpret_cast<uint32_t*>(GetPixelAddress(x, y));
}

const uint32_t* Bitmap::GetPixel32u(uint32_t x, uint32_t y) const
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_UINT32) {
		return nullptr;
	}
	return reinterpret_cast<const uint32_t*>(GetPixelAddress(x, y));
}

float* Bitmap::GetPixel32f(uint32_t x, uint32_t y)
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_FLOAT) {
		return nullptr;
	}
	return reinterpret_cast<float*>(GetPixelAddress(x, y));
}

const float* Bitmap::GetPixel32f(uint32_t x, uint32_t y) const
{
	if (Bitmap::ChannelDataType(m_format) != Bitmap::DATA_TYPE_FLOAT) {
		return nullptr;
	}
	return reinterpret_cast<const float*>(GetPixelAddress(x, y));
}

uint32_t Bitmap::ChannelSize(Bitmap::Format value)
{

	switch (value) {
	default: break;
	case Bitmap::FORMAT_R_UINT8: return 1; break;
	case Bitmap::FORMAT_RG_UINT8: return 1; break;
	case Bitmap::FORMAT_RGB_UINT8: return 1; break;
	case Bitmap::FORMAT_RGBA_UINT8: return 1; break;

	case Bitmap::FORMAT_R_UINT16: return 2; break;
	case Bitmap::FORMAT_RG_UINT16: return 2; break;
	case Bitmap::FORMAT_RGB_UINT16: return 2; break;
	case Bitmap::FORMAT_RGBA_UINT16: return 2; break;

	case Bitmap::FORMAT_R_UINT32: return 4; break;
	case Bitmap::FORMAT_RG_UINT32: return 4; break;
	case Bitmap::FORMAT_RGB_UINT32: return 4; break;
	case Bitmap::FORMAT_RGBA_UINT32: return 4; break;

	case Bitmap::FORMAT_R_FLOAT: return 4; break;
	case Bitmap::FORMAT_RG_FLOAT: return 4; break;
	case Bitmap::FORMAT_RGB_FLOAT: return 4; break;
	case Bitmap::FORMAT_RGBA_FLOAT: return 4; break;
	}

	return 0;
}

uint32_t Bitmap::ChannelCount(Bitmap::Format value)
{

	switch (value) {
	default: break;
	case Bitmap::FORMAT_R_UINT8: return 1; break;
	case Bitmap::FORMAT_RG_UINT8: return 2; break;
	case Bitmap::FORMAT_RGB_UINT8: return 3; break;
	case Bitmap::FORMAT_RGBA_UINT8: return 4; break;

	case Bitmap::FORMAT_R_UINT16: return 1; break;
	case Bitmap::FORMAT_RG_UINT16: return 2; break;
	case Bitmap::FORMAT_RGB_UINT16: return 3; break;
	case Bitmap::FORMAT_RGBA_UINT16: return 4; break;

	case Bitmap::FORMAT_R_UINT32: return 1; break;
	case Bitmap::FORMAT_RG_UINT32: return 2; break;
	case Bitmap::FORMAT_RGB_UINT32: return 3; break;
	case Bitmap::FORMAT_RGBA_UINT32: return 4; break;

	case Bitmap::FORMAT_R_FLOAT: return 1; break;
	case Bitmap::FORMAT_RG_FLOAT: return 2; break;
	case Bitmap::FORMAT_RGB_FLOAT: return 3; break;
	case Bitmap::FORMAT_RGBA_FLOAT: return 4; break;
	}

	return 0;
}

Bitmap::DataType Bitmap::ChannelDataType(Bitmap::Format value)
{

	switch (value) {
	default: break;
	case Bitmap::FORMAT_R_UINT8:
	case Bitmap::FORMAT_RG_UINT8:
	case Bitmap::FORMAT_RGB_UINT8:
	case Bitmap::FORMAT_RGBA_UINT8: {
		return Bitmap::DATA_TYPE_UINT8;
	} break;

	case Bitmap::FORMAT_R_UINT16:
	case Bitmap::FORMAT_RG_UINT16:
	case Bitmap::FORMAT_RGB_UINT16:
	case Bitmap::FORMAT_RGBA_UINT16: {
		return Bitmap::DATA_TYPE_UINT16;
	} break;

	case Bitmap::FORMAT_R_UINT32:
	case Bitmap::FORMAT_RG_UINT32:
	case Bitmap::FORMAT_RGB_UINT32:
	case Bitmap::FORMAT_RGBA_UINT32: {
		return Bitmap::DATA_TYPE_UINT32;
	} break;

	case Bitmap::FORMAT_R_FLOAT:
	case Bitmap::FORMAT_RG_FLOAT:
	case Bitmap::FORMAT_RGB_FLOAT:
	case Bitmap::FORMAT_RGBA_FLOAT: {
		return Bitmap::DATA_TYPE_FLOAT;
	} break;
	}

	return Bitmap::DATA_TYPE_UNDEFINED;
}

uint32_t Bitmap::FormatSize(Bitmap::Format value)
{
	uint32_t channelSize = Bitmap::ChannelSize(value);
	uint32_t channelCount = Bitmap::ChannelCount(value);
	uint32_t size = channelSize * channelCount;
	return size;
}

uint64_t Bitmap::StorageFootprint(uint32_t width, uint32_t height, Bitmap::Format format)
{
	uint64_t size = width * height * Bitmap::FormatSize(format);
	return size;
}

static Result IsRadianceFile(const std::filesystem::path& path, bool& isRadiance)
{
	// Open file
	File file;
	if (!file.Open(path.string().c_str())) {
		return ERROR_IMAGE_FILE_LOAD_FAILED;
	}
	// Signature buffer
	char buf[kRadianceSigSize] = { 0 };

	// Read signature
	size_t n = file.Read(buf, kRadianceSigSize);

	// Only check if kBufferSize bytes were read
	if (n == kRadianceSigSize) {
		int res = strncmp(buf, kRadianceSig, kRadianceSigSize);
		isRadiance = (res == 0);
	}

	return SUCCESS;
}

static Result IsRadianceImage(const size_t dataSize, const void* pData, bool& isRadiance)
{
	if ((dataSize < kRadianceSigSize) || IsNull(pData)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	int res = strncmp(static_cast<const char*>(pData), kRadianceSig, kRadianceSigSize);
	isRadiance = (res == 0);

	return SUCCESS;
}

Result Bitmap::StbiInfo(const std::filesystem::path& path, int* pX, int* pY, int* pComp)
{
	File file;
	if (!file.Open(path)) {
		return ERROR_IMAGE_FILE_LOAD_FAILED;
	}

	int stbiResult = 0;
	//if (file.IsMapped())
	//{
	//	stbiResult = stbi_info_from_memory(
	//		reinterpret_cast<const stbi_uc*>(file.GetMappedData()),
	//		static_cast<int>(file.GetLength()),
	//		pX,
	//		pY,
	//		pComp);
	//}
	//else 
	//{
	std::vector<uint8_t> buffer(file.GetLength());
	file.Read(buffer.data(), buffer.size());
	stbiResult = stbi_info_from_memory(
		buffer.data(),
		static_cast<int>(buffer.size()),
		pX,
		pY,
		pComp);
	//}

	return stbiResult ? SUCCESS : ERROR_IMAGE_FILE_LOAD_FAILED;
}

bool Bitmap::IsBitmapFile(const std::filesystem::path& path)
{
	int x, y, comp;
	return (StbiInfo(path, &x, &y, &comp) == SUCCESS);
}

Result Bitmap::GetFileProperties(const std::filesystem::path& path, uint32_t* pWidth, uint32_t* pHeight, Bitmap::Format* pFormat)
{
	if (!std::filesystem::exists(path)) return ERROR_PATH_DOES_NOT_EXIST;

	int x = 0;
	int y = 0;
	int comp = 0;
	StbiInfo(path, &x, &y, &comp);

	bool   isRadiance = false;
	Result ppxres = IsRadianceFile(path, isRadiance);
	if (Failed(ppxres)) {
		return ppxres;
	}

	if (!IsNull(pWidth)) {
		*pWidth = static_cast<uint32_t>(x);
	}

	if (!IsNull(pHeight)) {
		*pHeight = static_cast<uint32_t>(y);
	}

	// Force to 4 channels to make things easier for the graphics APIs.
	if (!IsNull(pFormat)) {
		*pFormat = isRadiance ? Bitmap::FORMAT_RGBA_FLOAT : Bitmap::FORMAT_RGBA_UINT8;
	}

	return SUCCESS;
}

char* Bitmap::StbiLoad(const std::filesystem::path& path, Bitmap::Format format, int* pWidth, int* pHeight, int* pChannels, int desiredChannels)
{
	File file;
	if (!file.Open(path)) {
		return nullptr;
	}

	std::vector<stbi_uc> buffer(0);
	//if (!file.IsMapped()) {
	buffer.resize(file.GetLength());
	file.Read(buffer.data(), buffer.size());
	//}

	const stbi_uc* readPtr = /*file.IsMapped() ? reinterpret_cast<const stbi_uc*>(file.GetMappedData()) :*/ buffer.data();
	if (format == Bitmap::FORMAT_RGBA_FLOAT) {
		return reinterpret_cast<char*>(stbi_loadf_from_memory(
			readPtr,
			static_cast<int>(file.GetLength()),
			pWidth,
			pHeight,
			pChannels,
			desiredChannels));
	}
	return reinterpret_cast<char*>(stbi_load_from_memory(
		readPtr,
		static_cast<int>(file.GetLength()),
		pWidth,
		pHeight,
		pChannels,
		desiredChannels));
}

Result Bitmap::LoadFile(const std::filesystem::path& path, Bitmap* pBitmap)
{
	if (!std::filesystem::exists(path)) return ERROR_PATH_DOES_NOT_EXIST;

	bool   isRadiance = false;
	Result ppxres = IsRadianceFile(path, isRadiance);
	if (Failed(ppxres)) {
		return ppxres;
	}

	int            width = 0;
	int            height = 0;
	int            channels = 0;
	int            requiredChannels = 4; // Force to 4 channels to make things easier for the graphics APIs.
	Bitmap::Format format = (isRadiance) ? Bitmap::FORMAT_RGBA_FLOAT : Bitmap::FORMAT_RGBA_UINT8;
	char* dataPtr = StbiLoad(path, format, &width, &height, &channels, requiredChannels);

	if (IsNull(dataPtr)) {
		Fatal("Failed to open file '" + path.string() + "'");
		return ERROR_IMAGE_FILE_LOAD_FAILED;
	}
	ppxres = Bitmap::Create(width, height, format, dataPtr, pBitmap);
	if (!pBitmap->IsOk()) {
		// Something has gone really wrong if this happens
		stbi_image_free(dataPtr);
		return ERROR_FAILED;
	}
	// Critical! This marks the memory as needing to be freed later!
	pBitmap->m_dataIsFromStbi = true;

	return SUCCESS;
}

Result Bitmap::SaveFilePNG(const std::filesystem::path& path, const Bitmap* pBitmap)
{
#if defined(_ANDROID)
	ASSERT_MSG(false, "SaveFilePNG not supported on Android");
	return ERROR_IMAGE_FILE_SAVE_FAILED;
#else
	int res = stbi_write_png(path.string().c_str(), pBitmap->GetWidth(), pBitmap->GetHeight(), pBitmap->GetChannelCount(), pBitmap->GetData(), pBitmap->GetRowStride());
	if (res == 0) {
		return ERROR_IMAGE_FILE_SAVE_FAILED;
	}
	return SUCCESS;
#endif
}

Result Bitmap::LoadFromMemory(const size_t dataSize, const void* pData, Bitmap* pBitmap)
{
	if ((dataSize == 0) || IsNull(pData) || IsNull(pBitmap)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	bool   isRadiance = false;
	Result ppxres = IsRadianceImage(dataSize, pData, isRadiance);
	if (Failed(ppxres)) {
		return ppxres;
	}

	int            width = 0;
	int            height = 0;
	int            channels = 0;
	int            requiredChannels = 4; // Force to 4 channels to make things easier for the graphics APIs.
	Bitmap::Format format = (isRadiance) ? Bitmap::FORMAT_RGBA_FLOAT : Bitmap::FORMAT_RGBA_UINT8;

	void* pStbData = nullptr;
	if (isRadiance) {
		pStbData = stbi_loadf_from_memory(
			static_cast<const stbi_uc*>(pData),
			static_cast<int>(dataSize),
			&width,
			&height,
			&channels,
			requiredChannels);
	}
	else {
		pStbData = stbi_load_from_memory(
			static_cast<const stbi_uc*>(pData),
			static_cast<int>(dataSize),
			&width,
			&height,
			&channels,
			requiredChannels);
	}

	if (IsNull(pStbData)) {
		return ERROR_IMAGE_FILE_LOAD_FAILED;
	}

	ppxres = Bitmap::Create(width, height, format, static_cast<char*>(pStbData), pBitmap);
	if (!pBitmap->IsOk()) {
		// Something has gone really wrong if this happens
		stbi_image_free(pStbData);
		return ERROR_FAILED;
	}
	// Critical! This marks the memory as needing to be freed later!
	pBitmap->m_dataIsFromStbi = true;

	return SUCCESS;
}

#pragma endregion
