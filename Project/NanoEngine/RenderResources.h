#pragma once

#pragma region Bitmap

class Bitmap
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

	// ---------------------------------------------------------------------------------------------

	Bitmap();
	Bitmap(const Bitmap& obj);
	~Bitmap();

	Bitmap& operator=(const Bitmap& rhs);

	//! Creates a bitmap with internal storage.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, Bitmap* pBitmap);
	//! Creates a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Bitmap* pBitmap);
	//! Creates a bitmap with external storage.
	static Result Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Bitmap* pBitmap);
	//! Returns a bitmap with internal storage.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, Result* pResult = nullptr);
	//! Returns a bitmap with external storage. If \b rowStride is 0, default row stride for format is used.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage, Result* pResult = nullptr);
	//! Returns a bitmap with external storage.
	static Bitmap Create(uint32_t width, uint32_t height, Bitmap::Format format, char* pExternalStorage, Result* pResult = nullptr);

	// Returns true if dimensions are greater than one, format is valid, and storage is valid
	bool IsOk() const;

	uint32_t       GetWidth() const { return mWidth; }
	uint32_t       GetHeight() const { return mHeight; }
	Bitmap::Format GetFormat() const { return mFormat; };
	uint32_t       GetChannelCount() const { return mChannelCount; }
	uint32_t       GetPixelStride() const { return mPixelStride; }
	uint32_t       GetRowStride() const { return mRowStride; }
	char* GetData() const { return mData; }
	uint64_t       GetFootprintSize(uint32_t rowStrideAlignment = 1) const;

	Result Resize(uint32_t width, uint32_t height);
	Result ScaleTo(Bitmap* pTargetBitmap) const;
	Result ScaleTo(Bitmap* pTargetBitmap, stbir_filter filterType) const;

	template <typename PixelDataType>
	void Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a);

	// Returns byte address of pixel at (x,y)
	char* GetPixelAddress(uint32_t x, uint32_t y);
	const char* GetPixelAddress(uint32_t x, uint32_t y) const;

	// These functions will return null if the Bitmap's format doesn't
	// the function. For example, GetPixel8u() returns null if the format
	// is FORMAT_RGBA_FLOAT.
	//
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

	// ---------------------------------------------------------------------------------------------

	class PixelIterator
	{
	private:
		friend class Bitmap;

		PixelIterator(Bitmap* pBitmap)
			: mBitmap(pBitmap)
		{
			Reset();
		}

	public:
		void Reset()
		{
			mX = 0;
			mY = 0;
			mPixelAddress = mBitmap->GetPixelAddress(mX, mY);
		}

		bool Done() const
		{
			bool done = (mY >= mBitmap->GetHeight());
			return done;
		}

		bool Next()
		{
			if (Done()) {
				return false;
			}

			mX += 1;
			mPixelAddress += mBitmap->GetPixelStride();
			if (mX == mBitmap->GetWidth()) {
				mY += 1;
				mX = 0;
				mPixelAddress = mBitmap->GetPixelAddress(mX, mY);
			}

			return Done() ? false : true;
		}

		uint32_t       GetX() const { return mX; }
		uint32_t       GetY() const { return mY; }
		Bitmap::Format GetFormat() const { return mBitmap->GetFormat(); }
		uint32_t       GetChannelCount() const { return Bitmap::ChannelCount(GetFormat()); }

		template <typename T>
		T* GetPixelAddress() const { return reinterpret_cast<T*>(mPixelAddress); }

	private:
		Bitmap* mBitmap = nullptr;
		uint32_t mX = 0;
		uint32_t mY = 0;
		char* mPixelAddress = nullptr;
	};

	// ---------------------------------------------------------------------------------------------

	PixelIterator GetPixelIterator() { return PixelIterator(this); }

private:
	void   InternalCtor();
	Result InternalInitialize(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t rowStride, char* pExternalStorage);
	Result InternalCopy(const Bitmap& obj);
	void   FreeStbiDataIfNeeded();

	// Stbi-specific functions/wrappers.
	// These arguments generally mirror those for stbi_load, except format which is used to determine whether
	// the call should be made to stbi_load or stbi_loadf (that is, whether the file to be read is in integer or floating point format).
	static char* StbiLoad(const std::filesystem::path& path, Bitmap::Format format, int* pWidth, int* pHeight, int* pChannels, int desiredChannels);
	// These arugments mirror those for stbi_info.
	static Result StbiInfo(const std::filesystem::path& path, int* pX, int* pY, int* pComp);

private:
	uint32_t          mWidth = 0;
	uint32_t          mHeight = 0;
	Bitmap::Format    mFormat = Bitmap::FORMAT_UNDEFINED;
	uint32_t          mChannelCount = 0;
	uint32_t          mPixelStride = 0;
	uint32_t          mRowStride = 0;
	char* mData = nullptr;
	bool              mDataIsFromStbi = false;
	std::vector<char> mInternalStorage = {};
};

template <typename PixelDataType>
void Bitmap::Fill(PixelDataType r, PixelDataType g, PixelDataType b, PixelDataType a)
{
	ASSERT_MSG(mData != nullptr, "data is null");
	ASSERT_MSG(mFormat != Bitmap::FORMAT_UNDEFINED, "format is undefined");

	PixelDataType rgba[4] = { r, g, b, a };

	const uint32_t channelCount = Bitmap::ChannelCount(mFormat);

	char* pData = mData;
	for (uint32_t y = 0; y < mHeight; ++y) {
		for (uint32_t x = 0; x < mWidth; ++x) {
			PixelDataType* pPixelData = reinterpret_cast<PixelDataType*>(pData);
			for (uint32_t c = 0; c < channelCount; ++c) {
				pPixelData[c] = rgba[c];
			}
			pData += mPixelStride;
		}
	}
}

#pragma endregion

#pragma region Font

struct FontMetrics
{
	float ascent = 0;
	float descent = 0;
	float lineGap = 0;
};

struct GlyphBox
{
	int32_t x0 = 0;
	int32_t y0 = 0;
	int32_t x1 = 0;
	int32_t y1 = 0;
};

struct GlyphMetrics
{
	float    advance = 0;
	float    leftBearing = 0;
	GlyphBox box = {};
};

class Font
{
public:
	Font();
	virtual ~Font();

	static Result CreateFromFile(const std::filesystem::path& path, Font* pFont);
	static Result CreateFromMemory(size_t size, const char* pData, Font* pFont);

	float GetScale(float fontSizeInPixels) const;

	void GetFontMetrics(float fontSizeInPixels, FontMetrics* pMetrics) const;

	void GetGlyphMetrics(
		float         fontSizeInPixels,
		uint32_t      codepoint,
		float         subpixelShiftX,
		float         subpixelShiftY,
		GlyphMetrics* pMetrics) const;

	void RenderGlyphBitmap(
		float          fontSizeInPixels,
		uint32_t       codepoint,
		float          subpixelShiftX,
		float          subpixelShiftY,
		uint32_t       glyphWidth,
		uint32_t       glyphHeight,
		uint32_t       rowStride,
		unsigned char* pOutput) const;

private:
	void AcquireFontMetrics();

private:
	struct Object
	{
		std::vector<unsigned char> fontData;
		stbtt_fontinfo             fontInfo;
		int                        ascent = 0;
		int                        descent = 0;
		int                        lineGap = 0;
	};
	std::shared_ptr<Object> mObject;
};

#pragma endregion

#pragma region Font Inconsolata

namespace imgui {

	extern const unsigned int  kFontInconsolataSize;
	extern const unsigned char kFontInconsolata[];

} // namespace imgui

#pragma endregion

#pragma region TriMesh

enum TriMeshAttributeDim
{
	TRI_MESH_ATTRIBUTE_DIM_UNDEFINED = 0,
	TRI_MESH_ATTRIBUTE_DIM_2 = 2,
	TRI_MESH_ATTRIBUTE_DIM_3 = 3,
	TRI_MESH_ATTRIBUTE_DIM_4 = 4,
};

//! @enum TriMeshPlane
//!
//!
enum TriMeshPlane
{
	TRI_MESH_PLANE_POSITIVE_X = 0,
	TRI_MESH_PLANE_NEGATIVE_X = 1,
	TRI_MESH_PLANE_POSITIVE_Y = 2,
	TRI_MESH_PLANE_NEGATIVE_Y = 3,
	TRI_MESH_PLANE_POSITIVE_Z = 4,
	TRI_MESH_PLANE_NEGATIVE_Z = 5,
};

//! @struct TriMeshVertexData
//!
//!
struct TriMeshVertexData
{
	float3 position;
	float3 color;
	float3 normal;
	float2 texCoord;
	float4 tangent;
	float3 bitangent;
};

//! @struct TriMeshVertexDataCompressed
//!
//!
struct TriMeshVertexDataCompressed
{
	half4  position;
	half3  color;
	i8vec4 normal;
	half2  texCoord;
	i8vec4 tangent;
	i8vec3 bitangent;
};

//! @class TriMeshOptions
//!
//!
class TriMeshOptions
{
public:
	TriMeshOptions() {}
	~TriMeshOptions() {}
	// clang-format off
	//! Enable/disable indices
	TriMeshOptions& Indices(bool value = true) { mEnableIndices = value; return *this; }
	//! Enable/disable vertex colors
	TriMeshOptions& VertexColors(bool value = true) { mEnableVertexColors = value; return *this; }
	//! Enable/disable normals
	TriMeshOptions& Normals(bool value = true) { mEnableNormals = value; return *this; }
	//! Enable/disable texture coordinates, most geometry will have 2-dimensional texture coordinates
	TriMeshOptions& TexCoords(bool value = true) { mEnableTexCoords = value; return *this; }
	//! Enable/disable tangent and bitangent creation flag
	TriMeshOptions& Tangents(bool value = true) { mEnableTangents = value; return *this; }
	//! Set and/or enable/disable object color, object color will override vertex colors
	TriMeshOptions& ObjectColor(const float3& color, bool enable = true) { mObjectColor = color; mEnableObjectColor = enable; return *this; }
	//! Set the translate of geometry position, default is (0, 0, 0)
	TriMeshOptions& Translate(const float3& translate) { mTranslate = translate; return *this; }
	//! Set the scale of geometry position, default is (1, 1, 1)
	TriMeshOptions& Scale(const float3& scale) { mScale = scale; return *this; }
	//! Sets the UV texture coordinate scale, default is (1, 1)
	TriMeshOptions& TexCoordScale(const float2& scale) { mTexCoordScale = scale; return *this; }
	//! Enable all attributes
	TriMeshOptions& AllAttributes() { mEnableVertexColors = true; mEnableNormals = true; mEnableTexCoords = true; mEnableTangents = true; return *this; }
	//! Inverts tex coords vertically
	TriMeshOptions& InvertTexCoordsV() { mInvertTexCoordsV = true; return *this; }
	//! Inverts winding order of ONLY indices
	TriMeshOptions& InvertWinding() { mInvertWinding = true; return *this; }
	// clang-format on
private:
	bool   mEnableIndices = false;
	bool   mEnableVertexColors = false;
	bool   mEnableNormals = false;
	bool   mEnableTexCoords = false;
	bool   mEnableTangents = false;
	bool   mEnableObjectColor = false;
	bool   mInvertTexCoordsV = false;
	bool   mInvertWinding = false;
	float3 mObjectColor = float3(0.7f);
	float3 mTranslate = float3(0, 0, 0);
	float3 mScale = float3(1, 1, 1);
	float2 mTexCoordScale = float2(1, 1);
	friend class TriMesh;
};

//! @class TriMesh
//!
//!
class TriMesh
{
public:
	TriMesh();
	TriMesh(grfx::IndexType indexType);
	TriMesh(TriMeshAttributeDim texCoordDim);
	TriMesh(grfx::IndexType indexType, TriMeshAttributeDim texCoordDim);
	~TriMesh();

	grfx::IndexType     GetIndexType() const { return mIndexType; }
	TriMeshAttributeDim GetTexCoordDim() const { return mTexCoordDim; }

	bool HasColors() const { return GetCountColors() > 0; }
	bool HasNormals() const { return GetCountNormals() > 0; }
	bool HasTexCoords() const { return GetCountTexCoords() > 0; }
	bool HasTangents() const { return GetCountTangents() > 0; }
	bool HasBitangents() const { return GetCountBitangents() > 0; }

	uint32_t GetCountTriangles() const;
	uint32_t GetCountIndices() const;
	uint32_t GetCountPositions() const;
	uint32_t GetCountColors() const;
	uint32_t GetCountNormals() const;
	uint32_t GetCountTexCoords() const;
	uint32_t GetCountTangents() const;
	uint32_t GetCountBitangents() const;

	uint64_t GetDataSizeIndices() const;
	uint64_t GetDataSizePositions() const;
	uint64_t GetDataSizeColors() const;
	uint64_t GetDataSizeNormalls() const;
	uint64_t GetDataSizeTexCoords() const;
	uint64_t GetDataSizeTangents() const;
	uint64_t GetDataSizeBitangents() const;

	const uint16_t* GetDataIndicesU16(uint32_t index = 0) const;
	const uint32_t* GetDataIndicesU32(uint32_t index = 0) const;
	const float3* GetDataPositions(uint32_t index = 0) const;
	const float3* GetDataColors(uint32_t index = 0) const;
	const float3* GetDataNormalls(uint32_t index = 0) const;
	const float2* GetDataTexCoords2(uint32_t index = 0) const;
	const float3* GetDataTexCoords3(uint32_t index = 0) const;
	const float4* GetDataTexCoords4(uint32_t index = 0) const;
	const float4* GetDataTangents(uint32_t index = 0) const;
	const float3* GetDataBitangents(uint32_t index = 0) const;

	const float3& GetBoundingBoxMin() const { return mBoundingBoxMin; }
	const float3& GetBoundingBoxMax() const { return mBoundingBoxMax; }

	// Preallocates triangle, position, color, normal, texture and tangent data (as desired) based on the provided triangle count.
	// Using this avoids doing those allocations (potentially multiple times) during the data load.
	void     PreallocateForTriangleCount(size_t triangleCount, bool enableColors, bool enableNormals, bool enableTexCoords, bool enableTangents);
	uint32_t AppendTriangle(uint32_t v0, uint32_t v1, uint32_t v2);
	uint32_t AppendPosition(const float3& value);
	uint32_t AppendColor(const float3& value);
	uint32_t AppendTexCoord(const float2& value);
	uint32_t AppendTexCoord(const float3& value);
	uint32_t AppendTexCoord(const float4& value);
	uint32_t AppendNormal(const float3& value);
	uint32_t AppendTangent(const float4& value);
	uint32_t AppendBitangent(const float3& value);

	Result GetTriangle(uint32_t triIndex, uint32_t& v0, uint32_t& v1, uint32_t& v2) const;
	Result GetVertexData(uint32_t vtxIndex, TriMeshVertexData* pVertexData) const;

	static TriMesh CreatePlane(TriMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options = TriMeshOptions());
	static TriMesh CreateCube(const float3& size, const TriMeshOptions& options = TriMeshOptions());
	static TriMesh CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const TriMeshOptions& options = TriMeshOptions());

	static Result  CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options, TriMesh* pTriMesh);
	static TriMesh CreateFromOBJ(const std::filesystem::path& path, const TriMeshOptions& options = TriMeshOptions());

private:
	void AppendIndexU16(uint16_t value);
	void AppendIndexU32(uint32_t value);

	static void AppendIndexAndVertexData(
		std::vector<uint32_t>& indexData,
		const std::vector<float>& vertexData,
		const uint32_t            expectedVertexCount,
		const TriMeshOptions& options,
		TriMesh& mesh);

private:
	grfx::IndexType      mIndexType = grfx::INDEX_TYPE_UNDEFINED;
	TriMeshAttributeDim  mTexCoordDim = TRI_MESH_ATTRIBUTE_DIM_UNDEFINED;
	std::vector<uint8_t> mIndices;        // Stores both 16 and 32 bit indices
	std::vector<float3>  mPositions;      // Vertex positions
	std::vector<float3>  mColors;         // Vertex colors
	std::vector<float3>  mNormals;        // Vertex normals
	std::vector<float>   mTexCoords;      // Vertex texcoords, dimension can be 2, 3, or 4
	std::vector<float4>  mTangents;       // Vertex tangents
	std::vector<float3>  mBitangents;     // Vertex bitangents
	float3               mBoundingBoxMin; // Bounding box min
	float3               mBoundingBoxMax; // Bounding box max
};

#pragma endregion

#pragma region WireMesh

enum WireMeshPlane
{
	WIRE_MESH_PLANE_POSITIVE_X = 0,
	WIRE_MESH_PLANE_NEGATIVE_X = 1,
	WIRE_MESH_PLANE_POSITIVE_Y = 2,
	WIRE_MESH_PLANE_NEGATIVE_Y = 3,
	WIRE_MESH_PLANE_POSITIVE_Z = 4,
	WIRE_MESH_PLANE_NEGATIVE_Z = 5,
};

//! @struct WireMeshVertexData
//!
//!
struct WireMeshVertexData
{
	float3 position;
	float3 color;
};

//! @class WireMeshOptions
//!
//!
class WireMeshOptions
{
public:
	WireMeshOptions() {}
	~WireMeshOptions() {}
	// clang-format off
	//! Enable/disable indices
	WireMeshOptions& Indices(bool value = true) { mEnableIndices = value; return *this; }
	//! Enable/disable vertex colors
	WireMeshOptions& VertexColors(bool value = true) { mEnableVertexColors = value; return *this; }
	//! Set and/or enable/disable object color, object color will override vertex colors
	WireMeshOptions& ObjectColor(const float3& color, bool enable = true) { mObjectColor = color; mEnableObjectColor = enable; return *this; }
	//! Set the scale of geometry position, default is (1, 1, 1)
	WireMeshOptions& Scale(const float3& scale) { mScale = scale; return *this; }
	// clang-format on
private:
	bool   mEnableIndices = false;
	bool   mEnableVertexColors = false;
	bool   mEnableObjectColor = false;
	float3 mObjectColor = float3(0.7f);
	float3 mScale = float3(1, 1, 1);
	friend class WireMesh;
};

//! @class WireMesh
//!
//!
class WireMesh
{
public:
	WireMesh();
	WireMesh(grfx::IndexType indexType);
	~WireMesh();

	grfx::IndexType GetIndexType() const { return mIndexType; }

	bool HasColors() const { return GetCountColors() > 0; }

	uint32_t GetCountEdges() const;
	uint32_t GetCountIndices() const;
	uint32_t GetCountPositions() const;
	uint32_t GetCountColors() const;

	uint64_t GetDataSizeIndices() const;
	uint64_t GetDataSizePositions() const;
	uint64_t GetDataSizeColors() const;

	const uint16_t* GetDataIndicesU16(uint32_t index = 0) const;
	const uint32_t* GetDataIndicesU32(uint32_t index = 0) const;
	const float3* GetDataPositions(uint32_t index = 0) const;
	const float3* GetDataColors(uint32_t index = 0) const;

	const float3& GetBoundingBoxMin() const { return mBoundingBoxMin; }
	const float3& GetBoundingBoxMax() const { return mBoundingBoxMax; }

	uint32_t AppendEdge(uint32_t v0, uint32_t v1);
	uint32_t AppendPosition(const float3& value);
	uint32_t AppendColor(const float3& value);

	Result GetEdge(uint32_t lineIndex, uint32_t& v0, uint32_t& v1) const;
	Result GetVertexData(uint32_t vtxIndex, WireMeshVertexData* pVertexData) const;

	static WireMesh CreatePlane(WireMeshPlane plane, const float2& size, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options = WireMeshOptions());
	static WireMesh CreateCube(const float3& size, const WireMeshOptions& options = WireMeshOptions());
	static WireMesh CreateSphere(float radius, uint32_t usegs, uint32_t vsegs, const WireMeshOptions& options = WireMeshOptions());

private:
	void AppendIndexU16(uint16_t value);
	void AppendIndexU32(uint32_t value);

	static void AppendIndexAndVertexData(
		std::vector<uint32_t>& indexData,
		const std::vector<float>& vertexData,
		const uint32_t            expectedVertexCount,
		const WireMeshOptions& options,
		WireMesh& mesh);

private:
	grfx::IndexType      mIndexType = grfx::INDEX_TYPE_UNDEFINED;
	std::vector<uint8_t> mIndices;        // Stores both 16 and 32 bit indices
	std::vector<float3>  mPositions;      // Vertex positions
	std::vector<float3>  mColors;         // Vertex colors
	float3               mBoundingBoxMin; // Bounding box min
	float3               mBoundingBoxMax; // Bounding box max
};

#pragma endregion

#pragma region Geometry

template <typename T>
class VertexDataProcessorBase;

enum GeometryVertexAttributeLayout
{
	GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED = 1,
	GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_PLANAR = 2,
	GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_POSITION_PLANAR = 3,
};

//! @struct GeometryCreateInfo
//!
//! primitiveTopology
//!   - only TRIANGLE_LIST is currently supported
//!
//! indexType
//!   - use UNDEFINED to indicate there's not index data
//!
//! attributeLayout
//!   - if INTERLEAVED only bindings[0] is used
//!   - if PLANAR bindings[0..bindingCount] are used
//!   - if PLANAR bindings can only have 1 attribute
//!
//! Supported vertex semantics:
//!    grfx::VERTEX_SEMANTIC_POSITION
//!    grfx::VERTEX_SEMANTIC_NORMAL
//!    grfx::VERTEX_SEMANTIC_COLOR
//!    grfx::VERTEX_SEMANTIC_TANGENT
//!    grfx::VERTEX_SEMANTIC_BITANGEN
//!    grfx::VERTEX_SEMANTIC_TEXCOORD
//!
struct GeometryCreateInfo
{
	grfx::IndexType               indexType = grfx::INDEX_TYPE_UNDEFINED;
	GeometryVertexAttributeLayout vertexAttributeLayout = GEOMETRY_VERTEX_ATTRIBUTE_LAYOUT_INTERLEAVED;
	uint32_t                      vertexBindingCount = 0;
	grfx::VertexBinding           vertexBindings[MAX_VERTEX_BINDINGS] = {};
	grfx::PrimitiveTopology       primitiveTopology = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Creates a create info objects with a UINT16 or UINT32 index
	// type and position vertex attribute.
	//
	static GeometryCreateInfo InterleavedU16(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo InterleavedU32(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PlanarU16(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PlanarU32(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PositionPlanarU16(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	static GeometryCreateInfo PositionPlanarU32(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);

	// Create a create info with a position vertex attribute.
	//
	static GeometryCreateInfo Interleaved();
	static GeometryCreateInfo Planar();
	static GeometryCreateInfo PositionPlanar();

	GeometryCreateInfo& IndexType(grfx::IndexType indexType_);
	GeometryCreateInfo& IndexTypeU16();
	GeometryCreateInfo& IndexTypeU32();

	// NOTE: Vertex input locations (Vulkan) are based on the order of
	//       when the attribute is added.
	//
	//       Example (assumes no attributes exist):
	//         AddPosition(); // location = 0
	//         AddColor();    // location = 1
	//
	//       Example (2 attributes exist):
	//         AddTexCoord(); // location = 2
	//         AddTangent();  // location = 3
	//
	// WARNING: Changing the attribute layout, index type, or messing
	//          with the vertex bindings after or in between calling
	//          these functions can result in undefined behavior.
	//
	GeometryCreateInfo& AddPosition(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	GeometryCreateInfo& AddNormal(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	GeometryCreateInfo& AddColor(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);
	GeometryCreateInfo& AddTexCoord(grfx::Format format = grfx::FORMAT_R32G32_FLOAT);
	GeometryCreateInfo& AddTangent(grfx::Format format = grfx::FORMAT_R32G32B32A32_FLOAT);
	GeometryCreateInfo& AddBitangent(grfx::Format format = grfx::FORMAT_R32G32B32_FLOAT);

private:
	GeometryCreateInfo& AddAttribute(grfx::VertexSemantic semantic, grfx::Format format);
};

//! @class Geometry
//!
//! Implementation Notes:
//!   - Recommended to avoid modifying the index/vertex buffers directly and to use
//!     the Append* functions instead (for smaller geometries especially)
//!
class Geometry
{
	template <typename T>
	friend class VertexDataProcessorBase;
	enum BufferType
	{
		BUFFER_TYPE_VERTEX = 1,
		BUFFER_TYPE_INDEX = 2,
	};

public:
	//! @class Buffer
	//!
	//! Element count is data size divided by element size.
	//! Example:
	//!   - If  buffer is storing 16 bit indices then element size
	//!     is 2 bytes. Dividing the value of GetSize() by 2 will
	//!     return the element count, i.e. the number of indices.
	//!
	//! There's two different ways to use Buffer. Mixing them will most
	//! likely lead to undefined behavior.
	//!
	//! Use #1:
	//!   - Create a buffer object with type and element size
	//!   - Call SetSize() to allocate storage
	//!   - Grab the pointer with GetData() and memcpy to buffer object
	//!
	//! Use #2:
	//!   - Create a buffer object with type and element size
	//!   - Call Append<T>() to append data to it
	//!
	class Buffer
	{
	public:
		Buffer() {}
		Buffer(BufferType type, uint32_t elementSize)
			: mType(type), mElementSize(elementSize) {}
		~Buffer() {}

		BufferType  GetType() const { return mType; }
		uint32_t    GetElementSize() const { return mElementSize; }
		uint32_t    GetSize() const { return static_cast<uint32_t>(mUsedSize); }
		char* GetData() { return DataPtr(mData); }
		const char* GetData() const { return DataPtr(mData); }
		uint32_t    GetElementCount() const;

		// Changes the buffer size to `size`.
		// If this operations grows the vector, this class considers all the elements
		// to be initialized. It is the caller responsibility to initialize those elements.
		void SetSize(uint32_t size)
		{
			mUsedSize = size;
			mData.resize(size);
		}

		// Trusts that calling code is well behaved :)
		template <typename T>
		void Append(const T& value)
		{
			Append(1, &value);
		}

		template <typename T>
		void Append(uint32_t count, const T* pValues)
		{
			uint32_t sizeOfValues = count * static_cast<uint32_t>(sizeof(T));
			if ((mUsedSize + sizeOfValues) > mData.size()) {
				mData.resize(std::max<size_t>(mUsedSize + sizeOfValues, mData.size() * 2));
			}

			//  Copy data
			const void* pSrc = pValues;
			void* pDst = mData.data() + mUsedSize;
			memcpy(pDst, pSrc, sizeOfValues);
			mUsedSize += sizeOfValues;
		}

	private:
		BufferType        mType = BUFFER_TYPE_VERTEX;
		uint32_t          mElementSize = 0;
		size_t            mUsedSize = 0;
		std::vector<char> mData;
	};

	// ---------------------------------------------------------------------------------------------
public:
	Geometry() {}
	virtual ~Geometry() {}

private:
	Result InternalCtor();

public:
	// Create object using parameters from createInfo
	static Result Create(const GeometryCreateInfo& createInfo, Geometry* pGeometry);

	// Create object using parameters from createInfo using data from mesh
	static Result Create(
		const GeometryCreateInfo& createInfo,
		const TriMesh& mesh,
		Geometry* pGeometry);

	// Create object using parameters from createInfo using data from tri mesh
	static Result Create(
		const GeometryCreateInfo& createInfo,
		const WireMesh& mesh,
		Geometry* pGeometry);

	// Create object with a create info derived from mesh
	static Result Create(const TriMesh& mesh, Geometry* pGeometry);
	static Result Create(const WireMesh& mesh, Geometry* pGeometry);

	grfx::IndexType         GetIndexType() const { return mCreateInfo.indexType; }
	const Geometry::Buffer* GetIndexBuffer() const { return &mIndexBuffer; }
	void                    SetIndexBuffer(const Geometry::Buffer& newIndexBuffer);
	uint32_t                GetIndexCount() const;

	GeometryVertexAttributeLayout GetVertexAttributeLayout() const { return mCreateInfo.vertexAttributeLayout; }
	uint32_t                      GetVertexBindingCount() const { return mCreateInfo.vertexBindingCount; }
	const grfx::VertexBinding* GetVertexBinding(uint32_t index) const;

	uint32_t                GetVertexCount() const;
	uint32_t                GetVertexBufferCount() const { return CountU32(mVertexBuffers); }
	const Geometry::Buffer* GetVertexBuffer(uint32_t index) const;
	Geometry::Buffer* GetVertexBuffer(uint32_t index);
	uint32_t                GetLargestBufferSize() const;

	// Appends single index, triangle, or edge vertex indices to index buffer
	//
	// Will cast to uint16_t if geometry index type is UINT16.
	// NOOP if index type is UNDEFINED (geometry does not have index data).
	//
	void AppendIndex(uint32_t idx);
	void AppendIndicesTriangle(uint32_t idx0, uint32_t idx1, uint32_t idx2);
	void AppendIndicesEdge(uint32_t idx0, uint32_t idx1);

	// Append a chunk of UINT32 indices
	void AppendIndicesU32(uint32_t count, const uint32_t* pIndices);

	// Append multiple attributes at once
	//
	uint32_t AppendVertexData(const TriMeshVertexData& vtx);
	uint32_t AppendVertexData(const TriMeshVertexDataCompressed& vtx);
	uint32_t AppendVertexData(const WireMeshVertexData& vtx);

	// Appends triangle or edge vertex data and indices (if present)
	//
	//
	void AppendTriangle(const TriMeshVertexData& vtx0, const TriMeshVertexData& vtx1, const TriMeshVertexData& vtx2);
	void AppendEdge(const WireMeshVertexData& vtx0, const WireMeshVertexData& vtx1);

private:
	// This is intialized to point to a static var of derived class of VertexDataProcessorBase
	// which is shared by geometry objects, it is not supposed to be deleted
	VertexDataProcessorBase<TriMeshVertexData>* mVDProcessor = nullptr;
	VertexDataProcessorBase<TriMeshVertexDataCompressed>* mVDProcessorCompressed = nullptr;
	GeometryCreateInfo                                    mCreateInfo = {};
	Geometry::Buffer                                      mIndexBuffer;
	std::vector<Geometry::Buffer>                         mVertexBuffers;
	uint32_t                                              mPositionBufferIndex = VALUE_IGNORED;
	uint32_t                                              mNormaBufferIndex = VALUE_IGNORED;
	uint32_t                                              mColorBufferIndex = VALUE_IGNORED;
	uint32_t                                              mTexCoordBufferIndex = VALUE_IGNORED;
	uint32_t                                              mTangentBufferIndex = VALUE_IGNORED;
	uint32_t                                              mBitangentBufferIndex = VALUE_IGNORED;
};

#pragma endregion

#pragma region Sync

namespace grfx {

	//! @struct FenceCreateInfo
	//!
	//!
	struct FenceCreateInfo
	{
		bool signaled = false;
	};

	//! @class Fence
	//!
	//!
	class Fence
		: public grfx::DeviceObject<grfx::FenceCreateInfo>
	{
	public:
		Fence() {}
		virtual ~Fence() {}

		virtual Result Wait(uint64_t timeout = UINT64_MAX) = 0;
		virtual Result Reset() = 0;

		Result WaitAndReset(uint64_t timeout = UINT64_MAX);

	protected:
		virtual Result CreateApiObjects(const grfx::FenceCreateInfo* pCreateInfo) = 0;
		virtual void   DestroyApiObjects() = 0;
		friend class grfx::Device;
	};

	// -------------------------------------------------------------------------------------------------

	//! @struct SemaphoreCreateInfo
	//!
	//!
	struct SemaphoreCreateInfo
	{
		grfx::SemaphoreType semaphoreType = grfx::SEMAPHORE_TYPE_BINARY;
		uint64_t            initialValue = 0; // Timeline semaphore only
	};

	//! @class Semaphore
	//!
	//!
	class Semaphore
		: public grfx::DeviceObject<grfx::SemaphoreCreateInfo>
	{
	public:
		Semaphore() {}
		virtual ~Semaphore() {}

		grfx::SemaphoreType GetSemaphoreType() const { return mCreateInfo.semaphoreType; }
		bool                IsBinary() const { return mCreateInfo.semaphoreType == grfx::SEMAPHORE_TYPE_BINARY; }
		bool                IsTimeline() const { return mCreateInfo.semaphoreType == grfx::SEMAPHORE_TYPE_TIMELINE; }

		// Timeline semaphore wait
		Result Wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;

		// Timeline semaphore signal
		//
		// WARNING: Signaling a value less than what's already been signaled can
		//          cause a block or a race condition.
		//
		Result Signal(uint64_t value) const;

		// Returns current timeline semaphore value
		uint64_t GetCounterValue() const;

	private:
		virtual Result   TimelineWait(uint64_t value, uint64_t timeout) const = 0;
		virtual Result   TimelineSignal(uint64_t value) const = 0;
		virtual uint64_t TimelineCounterValue() const = 0;

	protected:
		virtual Result CreateApiObjects(const grfx::SemaphoreCreateInfo* pCreateInfo) = 0;
		virtual void   DestroyApiObjects() = 0;
		friend class grfx::Device;
	};

	// -------------------------------------------------------------------------------------------------

} // namespace grfx

#pragma endregion

#pragma region Query

#define GRFX_PIPELINE_STATISTIC_NUM_ENTRIES 11

namespace grfx {

	union PipelineStatistics
	{
		struct
		{
			uint64_t IAVertices;    // Input Assembly Vertices
			uint64_t IAPrimitives;  // Input Assembly Primitives
			uint64_t VSInvocations; // Vertex Shader Invocations
			uint64_t GSInvocations; // Geometry Shader Invocations
			uint64_t GSPrimitives;  // Geometry Shader Primitives
			uint64_t CInvocations;  // Clipping Invocations
			uint64_t CPrimitives;   // Clipping Primitives
			uint64_t PSInvocations; // Pixel Shader Invocations
			uint64_t HSInvocations; // Hull Shader Invocations
			uint64_t DSInvocations; // Domain Shader Invocations
			uint64_t CSInvocations; // Compute Shader Invocations
		};
		uint64_t Statistics[GRFX_PIPELINE_STATISTIC_NUM_ENTRIES] = { 0 };
	};

	//! @struct QueryCreateInfo
	//!
	//!
	struct QueryCreateInfo
	{
		grfx::QueryType type = QUERY_TYPE_UNDEFINED;
		uint32_t        count = 0;
	};

	//! @class Query
	//!
	//!
	class Query
		: public grfx::DeviceObject<grfx::QueryCreateInfo>
	{
	public:
		Query() {}
		virtual ~Query() {}

		grfx::QueryType GetType() const { return mCreateInfo.type; }
		uint32_t        GetCount() const { return mCreateInfo.count; }

		virtual void   Reset(uint32_t firstQuery, uint32_t queryCount) = 0;
		virtual Result GetData(void* pDstData, uint64_t dstDataSize) = 0;

	protected:
		virtual Result Create(const grfx::QueryCreateInfo* pCreateInfo) override;
		friend class grfx::Device;
	};

} // namespace grfx

#pragma endregion

#pragma region Buffer

namespace grfx {

	struct BufferCreateInfo
	{
		uint64_t               size = 0;
		uint32_t               structuredElementStride = 0; // HLSL StructuredBuffer<> only
		grfx::BufferUsageFlags usageFlags = 0;
		grfx::MemoryUsage      memoryUsage = grfx::MEMORY_USAGE_GPU_ONLY;
		grfx::ResourceState    initialState = grfx::RESOURCE_STATE_GENERAL;
		grfx::Ownership        ownership = grfx::OWNERSHIP_REFERENCE;
	};

	class Buffer : public grfx::DeviceObject<grfx::BufferCreateInfo>
	{
	public:
		Buffer() {}
		virtual ~Buffer() {}

		uint64_t                      GetSize() const { return mCreateInfo.size; }
		uint32_t                      GetStructuredElementStride() const { return mCreateInfo.structuredElementStride; }
		const grfx::BufferUsageFlags& GetUsageFlags() const { return mCreateInfo.usageFlags; }

		virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) = 0;
		virtual void   UnmapMemory() = 0;

		Result CopyFromSource(uint32_t dataSize, const void* pData);
		Result CopyToDest(uint32_t dataSize, void* pData);

	private:
		virtual Result Create(const grfx::BufferCreateInfo* pCreateInfo) override;
		friend class grfx::Device;
	};

	// -------------------------------------------------------------------------------------------------

	struct IndexBufferView
	{
		const grfx::Buffer* pBuffer = nullptr;
		grfx::IndexType     indexType = grfx::INDEX_TYPE_UINT16;
		uint64_t            offset = 0;
		uint64_t            size = WHOLE_SIZE; // [D3D12 - REQUIRED] Size in bytes of view

		IndexBufferView() {}

		IndexBufferView(const grfx::Buffer* pBuffer_, grfx::IndexType indexType_, uint64_t offset_ = 0, uint64_t size_ = WHOLE_SIZE)
			: pBuffer(pBuffer_), indexType(indexType_), offset(offset_), size(size_) {}
	};

	// -------------------------------------------------------------------------------------------------

	struct VertexBufferView
	{
		const grfx::Buffer* pBuffer = nullptr;
		uint32_t            stride = 0; // [D3D12 - REQUIRED] Stride in bytes of vertex entry
		uint64_t            offset = 0;
		uint64_t            size = WHOLE_SIZE; // [D3D12 - REQUIRED] Size in bytes of view

		VertexBufferView() {}

		VertexBufferView(const grfx::Buffer* pBuffer_, uint32_t stride_, uint64_t offset_ = 0, uint64_t size_ = 0)
			: pBuffer(pBuffer_), stride(stride_), offset(offset_), size(size_) {}
	};

} // namespace grfx

#pragma endregion

#pragma region Image


namespace grfx
{

	struct ImageCreateInfo
	{
		grfx::ImageType              type = grfx::IMAGE_TYPE_2D;
		uint32_t                     width = 0;
		uint32_t                     height = 0;
		uint32_t                     depth = 0;
		grfx::Format                 format = grfx::FORMAT_UNDEFINED;
		grfx::SampleCount            sampleCount = grfx::SAMPLE_COUNT_1;
		uint32_t                     mipLevelCount = 1;
		uint32_t                     arrayLayerCount = 1;
		grfx::ImageUsageFlags        usageFlags = grfx::ImageUsageFlags::SampledImage();
		grfx::MemoryUsage            memoryUsage = grfx::MEMORY_USAGE_GPU_ONLY;  // D3D12 will fail on any other memory usage
		grfx::ResourceState          initialState = grfx::RESOURCE_STATE_GENERAL; // This may not be the best choice
		grfx::RenderTargetClearValue RTVClearValue = { 0, 0, 0, 0 };                 // Optimized RTV clear value
		grfx::DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };                 // Optimized DSV clear value
		void* pApiObject = nullptr;                      // [OPTIONAL] For external images such as swapchain images
		grfx::Ownership              ownership = grfx::OWNERSHIP_REFERENCE;
		bool                         concurrentMultiQueueUsage = false;
		grfx::ImageCreateFlags       createFlags = {};

		// Returns a create info for sampled image
		static ImageCreateInfo SampledImage2D(
			uint32_t          width,
			uint32_t          height,
			grfx::Format      format,
			grfx::SampleCount sampleCount = grfx::SAMPLE_COUNT_1,
			grfx::MemoryUsage memoryUsage = grfx::MEMORY_USAGE_GPU_ONLY);

		// Returns a create info for sampled image and depth stencil target
		static ImageCreateInfo DepthStencilTarget(
			uint32_t          width,
			uint32_t          height,
			grfx::Format      format,
			grfx::SampleCount sampleCount = grfx::SAMPLE_COUNT_1);

		// Returns a create info for sampled image and render target
		static ImageCreateInfo RenderTarget2D(
			uint32_t          width,
			uint32_t          height,
			grfx::Format      format,
			grfx::SampleCount sampleCount = grfx::SAMPLE_COUNT_1);
	};

	class Image
		: public grfx::DeviceObject<grfx::ImageCreateInfo>
	{
	public:
		Image() {}
		virtual ~Image() {}

		grfx::ImageType                     GetType() const { return mCreateInfo.type; }
		uint32_t                            GetWidth() const { return mCreateInfo.width; }
		uint32_t                            GetHeight() const { return mCreateInfo.height; }
		uint32_t                            GetDepth() const { return mCreateInfo.depth; }
		grfx::Format                        GetFormat() const { return mCreateInfo.format; }
		grfx::SampleCount                   GetSampleCount() const { return mCreateInfo.sampleCount; }
		uint32_t                            GetMipLevelCount() const { return mCreateInfo.mipLevelCount; }
		uint32_t                            GetArrayLayerCount() const { return mCreateInfo.arrayLayerCount; }
		const grfx::ImageUsageFlags& GetUsageFlags() const { return mCreateInfo.usageFlags; }
		grfx::MemoryUsage                   GetMemoryUsage() const { return mCreateInfo.memoryUsage; }
		grfx::ResourceState                 GetInitialState() const { return mCreateInfo.initialState; }
		const grfx::RenderTargetClearValue& GetRTVClearValue() const { return mCreateInfo.RTVClearValue; }
		const grfx::DepthStencilClearValue& GetDSVClearValue() const { return mCreateInfo.DSVClearValue; }
		bool                                GetConcurrentMultiQueueUsageEnabled() const { return mCreateInfo.concurrentMultiQueueUsage; }
		grfx::ImageCreateFlags              GetCreateFlags() const { return mCreateInfo.createFlags; }

		// Convenience functions
		grfx::ImageViewType GuessImageViewType(bool isCube = false) const;

		virtual Result MapMemory(uint64_t offset, void** ppMappedAddress) = 0;
		virtual void   UnmapMemory() = 0;

	protected:
		virtual Result Create(const grfx::ImageCreateInfo* pCreateInfo) override;
		friend class grfx::Device;
	};

	namespace internal {

		class ImageResourceView
		{
		public:
			ImageResourceView() {}
			virtual ~ImageResourceView() {}
		};

	} // namespace internal

	//! @class ImageView
	//!
	//! This class exists to genericize descriptor updates for Vulkan's 'image' based resources.
	//!
	class ImageView
	{
	public:
		ImageView() {}
		virtual ~ImageView() {}

		const grfx::internal::ImageResourceView* GetResourceView() const { return mResourceView.get(); }

	protected:
		void SetResourceView(std::unique_ptr<internal::ImageResourceView>&& view)
		{
			mResourceView = std::move(view);
		}

	private:
		std::unique_ptr<internal::ImageResourceView> mResourceView;
	};

	struct SamplerCreateInfo
	{
		grfx::Filter             magFilter = grfx::FILTER_NEAREST;
		grfx::Filter             minFilter = grfx::FILTER_NEAREST;
		grfx::SamplerMipmapMode  mipmapMode = grfx::SAMPLER_MIPMAP_MODE_NEAREST;
		grfx::SamplerAddressMode addressModeU = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
		grfx::SamplerAddressMode addressModeV = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
		grfx::SamplerAddressMode addressModeW = grfx::SAMPLER_ADDRESS_MODE_REPEAT;
		float                    mipLodBias = 0.0f;
		bool                     anisotropyEnable = false;
		float                    maxAnisotropy = 0.0f;
		bool                     compareEnable = false;
		grfx::CompareOp          compareOp = grfx::COMPARE_OP_NEVER;
		float                    minLod = 0.0f;
		float                    maxLod = 1.0f;
		grfx::BorderColor        borderColor = grfx::BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		SamplerYcbcrConversion* pYcbcrConversion = nullptr; // Leave null if not required.
		grfx::Ownership          ownership = grfx::OWNERSHIP_REFERENCE;
		grfx::SamplerCreateFlags createFlags = {};
	};

	class Sampler
		: public grfx::DeviceObject<grfx::SamplerCreateInfo>
	{
	public:
		Sampler() {}
		virtual ~Sampler() {}
	};

	struct DepthStencilViewCreateInfo
	{
		grfx::Image* pImage = nullptr;
		grfx::ImageViewType     imageViewType = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
		grfx::Format            format = grfx::FORMAT_UNDEFINED;
		uint32_t                mipLevel = 0;
		uint32_t                mipLevelCount = 0;
		uint32_t                arrayLayer = 0;
		uint32_t                arrayLayerCount = 0;
		grfx::ComponentMapping  components = {};
		grfx::AttachmentLoadOp  depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
		grfx::AttachmentStoreOp depthStoreOp = ATTACHMENT_STORE_OP_STORE;
		grfx::AttachmentLoadOp  stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;
		grfx::AttachmentStoreOp stencilStoreOp = ATTACHMENT_STORE_OP_STORE;
		grfx::Ownership         ownership = grfx::OWNERSHIP_REFERENCE;

		static grfx::DepthStencilViewCreateInfo GuessFromImage(grfx::Image* pImage);
	};

	class DepthStencilView
		: public grfx::DeviceObject<grfx::DepthStencilViewCreateInfo>,
		public grfx::ImageView
	{
	public:
		DepthStencilView() {}
		virtual ~DepthStencilView() {}

		grfx::ImagePtr          GetImage() const { return mCreateInfo.pImage; }
		grfx::Format            GetFormat() const { return mCreateInfo.format; }
		grfx::SampleCount       GetSampleCount() const { return GetImage()->GetSampleCount(); }
		uint32_t                GetMipLevel() const { return mCreateInfo.mipLevel; }
		uint32_t                GetArrayLayer() const { return mCreateInfo.arrayLayer; }
		grfx::AttachmentLoadOp  GetDepthLoadOp() const { return mCreateInfo.depthLoadOp; }
		grfx::AttachmentStoreOp GetDepthStoreOp() const { return mCreateInfo.depthStoreOp; }
		grfx::AttachmentLoadOp  GetStencilLoadOp() const { return mCreateInfo.stencilLoadOp; }
		grfx::AttachmentStoreOp GetStencilStoreOp() const { return mCreateInfo.stencilStoreOp; }
	};

	struct RenderTargetViewCreateInfo
	{
		grfx::Image* pImage = nullptr;
		grfx::ImageViewType     imageViewType = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
		grfx::Format            format = grfx::FORMAT_UNDEFINED;
		uint32_t                mipLevel = 0;
		uint32_t                mipLevelCount = 0;
		uint32_t                arrayLayer = 0;
		uint32_t                arrayLayerCount = 0;
		grfx::ComponentMapping  components = {};
		grfx::AttachmentLoadOp  loadOp = ATTACHMENT_LOAD_OP_LOAD;
		grfx::AttachmentStoreOp storeOp = ATTACHMENT_STORE_OP_STORE;
		grfx::Ownership         ownership = grfx::OWNERSHIP_REFERENCE;

		static grfx::RenderTargetViewCreateInfo GuessFromImage(grfx::Image* pImage);
	};

	class RenderTargetView
		: public grfx::DeviceObject<grfx::RenderTargetViewCreateInfo>,
		public grfx::ImageView
	{
	public:
		RenderTargetView() {}
		virtual ~RenderTargetView() {}

		grfx::ImagePtr          GetImage() const { return mCreateInfo.pImage; }
		grfx::Format            GetFormat() const { return mCreateInfo.format; }
		grfx::SampleCount       GetSampleCount() const { return GetImage()->GetSampleCount(); }
		uint32_t                GetMipLevel() const { return mCreateInfo.mipLevel; }
		uint32_t                GetArrayLayer() const { return mCreateInfo.arrayLayer; }
		grfx::AttachmentLoadOp  GetLoadOp() const { return mCreateInfo.loadOp; }
		grfx::AttachmentStoreOp GetStoreOp() const { return mCreateInfo.storeOp; }
	};

	struct SampledImageViewCreateInfo
	{
		grfx::Image* pImage = nullptr;
		grfx::ImageViewType     imageViewType = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
		grfx::Format            format = grfx::FORMAT_UNDEFINED;
		grfx::SampleCount       sampleCount = grfx::SAMPLE_COUNT_1;
		uint32_t                mipLevel = 0;
		uint32_t                mipLevelCount = 0;
		uint32_t                arrayLayer = 0;
		uint32_t                arrayLayerCount = 0;
		grfx::ComponentMapping  components = {};
		SamplerYcbcrConversion* pYcbcrConversion = nullptr; // Leave null if not required.
		grfx::Ownership         ownership = grfx::OWNERSHIP_REFERENCE;

		static grfx::SampledImageViewCreateInfo GuessFromImage(grfx::Image* pImage);
	};

	class SampledImageView
		: public grfx::DeviceObject<grfx::SampledImageViewCreateInfo>,
		public grfx::ImageView
	{
	public:
		SampledImageView() {}
		virtual ~SampledImageView() {}

		grfx::ImagePtr                GetImage() const { return mCreateInfo.pImage; }
		grfx::ImageViewType           GetImageViewType() const { return mCreateInfo.imageViewType; }
		grfx::Format                  GetFormat() const { return mCreateInfo.format; }
		grfx::SampleCount             GetSampleCount() const { return GetImage()->GetSampleCount(); }
		uint32_t                      GetMipLevel() const { return mCreateInfo.mipLevel; }
		uint32_t                      GetMipLevelCount() const { return mCreateInfo.mipLevelCount; }
		uint32_t                      GetArrayLayer() const { return mCreateInfo.arrayLayer; }
		uint32_t                      GetArrayLayerCount() const { return mCreateInfo.arrayLayerCount; }
		const grfx::ComponentMapping& GetComponents() const { return mCreateInfo.components; }
	};

	// SamplerYcbcrConversionCreateInfo defines a color model conversion for a
	// texture, sampler, or sampled image.
	struct SamplerYcbcrConversionCreateInfo
	{
		grfx::Format               format = grfx::FORMAT_UNDEFINED;
		grfx::YcbcrModelConversion ycbcrModel = grfx::YCBCR_MODEL_CONVERSION_RGB_IDENTITY;
		grfx::YcbcrRange           ycbcrRange = grfx::YCBCR_RANGE_ITU_FULL;
		grfx::ComponentMapping     components = {};
		grfx::ChromaLocation       xChromaOffset = grfx::CHROMA_LOCATION_COSITED_EVEN;
		grfx::ChromaLocation       yChromaOffset = grfx::CHROMA_LOCATION_COSITED_EVEN;
		grfx::Filter               filter = grfx::FILTER_LINEAR;
		bool                       forceExplicitReconstruction = false;
	};

	class SamplerYcbcrConversion
		: public grfx::DeviceObject<grfx::SamplerYcbcrConversionCreateInfo>
	{
	public:
		SamplerYcbcrConversion() {}
		virtual ~SamplerYcbcrConversion() {}
	};

	struct StorageImageViewCreateInfo
	{
		grfx::Image* pImage = nullptr;
		grfx::ImageViewType    imageViewType = grfx::IMAGE_VIEW_TYPE_UNDEFINED;
		grfx::Format           format = grfx::FORMAT_UNDEFINED;
		uint32_t               mipLevel = 0;
		uint32_t               mipLevelCount = 0;
		uint32_t               arrayLayer = 0;
		uint32_t               arrayLayerCount = 0;
		grfx::ComponentMapping components = {};
		grfx::Ownership        ownership = grfx::OWNERSHIP_REFERENCE;

		static grfx::StorageImageViewCreateInfo GuessFromImage(grfx::Image* pImage);
	};

	class StorageImageView
		: public grfx::DeviceObject<grfx::StorageImageViewCreateInfo>,
		public grfx::ImageView
	{
	public:
		StorageImageView() {}
		virtual ~StorageImageView() {}

		grfx::ImagePtr      GetImage() const { return mCreateInfo.pImage; }
		grfx::ImageViewType GetImageViewType() const { return mCreateInfo.imageViewType; }
		grfx::Format        GetFormat() const { return mCreateInfo.format; }
		grfx::SampleCount   GetSampleCount() const { return GetImage()->GetSampleCount(); }
		uint32_t            GetMipLevel() const { return mCreateInfo.mipLevel; }
		uint32_t            GetMipLevelCount() const { return mCreateInfo.mipLevelCount; }
		uint32_t            GetArrayLayer() const { return mCreateInfo.arrayLayer; }
		uint32_t            GetArrayLayerCount() const { return mCreateInfo.arrayLayerCount; }
	};

} // namespace grfx

#pragma endregion

#pragma region Texture

namespace grfx {

	//! @struct TextureCreateInfo
	//!
	//!
	struct TextureCreateInfo
	{
		grfx::Image* pImage = nullptr;
		grfx::ImageType               imageType = grfx::IMAGE_TYPE_2D;
		uint32_t                      width = 0;
		uint32_t                      height = 0;
		uint32_t                      depth = 0;
		grfx::Format                  imageFormat = grfx::FORMAT_UNDEFINED;
		grfx::SampleCount             sampleCount = grfx::SAMPLE_COUNT_1;
		uint32_t                      mipLevelCount = 1;
		uint32_t                      arrayLayerCount = 1;
		grfx::ImageUsageFlags         usageFlags = grfx::ImageUsageFlags::SampledImage();
		grfx::MemoryUsage             memoryUsage = grfx::MEMORY_USAGE_GPU_ONLY;
		grfx::ResourceState           initialState = grfx::RESOURCE_STATE_GENERAL;    // This may not be the best choice
		grfx::RenderTargetClearValue  RTVClearValue = { 0, 0, 0, 0 };                    // Optimized RTV clear value
		grfx::DepthStencilClearValue  DSVClearValue = { 1.0f, 0xFF };                    // Optimized DSV clear value
		grfx::ImageViewType           sampledImageViewType = grfx::IMAGE_VIEW_TYPE_UNDEFINED; // Guesses from image if UNDEFINED
		grfx::Format                  sampledImageViewFormat = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
		grfx::SamplerYcbcrConversion* pSampledImageYcbcrConversion = nullptr;                         // Leave null if not Ycbcr, or not using sampled image.
		grfx::Format                  renderTargetViewFormat = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
		grfx::Format                  depthStencilViewFormat = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
		grfx::Format                  storageImageViewFormat = grfx::FORMAT_UNDEFINED;          // Guesses from image if UNDEFINED
		grfx::Ownership               ownership = grfx::OWNERSHIP_REFERENCE;
		bool                          concurrentMultiQueueUsage = false;
		grfx::ImageCreateFlags        imageCreateFlags = {};
	};

	//! @class Texture
	//!
	//!
	class Texture
		: public grfx::DeviceObject<grfx::TextureCreateInfo>
	{
	public:
		Texture() {}
		virtual ~Texture() {}

		grfx::ImageType              GetImageType() const;
		uint32_t                     GetWidth() const;
		uint32_t                     GetHeight() const;
		uint32_t                     GetDepth() const;
		grfx::Format                 GetImageFormat() const;
		grfx::SampleCount            GetSampleCount() const;
		uint32_t                     GetMipLevelCount() const;
		uint32_t                     GetArrayLayerCount() const;
		const grfx::ImageUsageFlags& GetUsageFlags() const;
		grfx::MemoryUsage            GetMemoryUsage() const;

		grfx::Format GetSampledImageViewFormat() const;
		grfx::Format GetRenderTargetViewFormat() const;
		grfx::Format GetDepthStencilViewFormat() const;
		grfx::Format GetStorageImageViewFormat() const;

		grfx::ImagePtr            GetImage() const { return mImage; }
		grfx::SampledImageViewPtr GetSampledImageView() const { return mSampledImageView; }
		grfx::RenderTargetViewPtr GetRenderTargetView() const { return mRenderTargetView; }
		grfx::DepthStencilViewPtr GetDepthStencilView() const { return mDepthStencilView; }
		grfx::StorageImageViewPtr GetStorageImageView() const { return mStorageImageView; }

	protected:
		virtual Result Create(const grfx::TextureCreateInfo* pCreateInfo) override;
		friend class grfx::Device;

		virtual Result CreateApiObjects(const grfx::TextureCreateInfo* pCreateInfo) override;
		virtual void   DestroyApiObjects() override;

	private:
		grfx::ImagePtr            mImage;
		grfx::SampledImageViewPtr mSampledImageView;
		grfx::RenderTargetViewPtr mRenderTargetView;
		grfx::DepthStencilViewPtr mDepthStencilView;
		grfx::StorageImageViewPtr mStorageImageView;
	};

} // namespace grfx

#pragma endregion

#pragma region Descriptor

namespace grfx {

	//! @struct DescriptorBinding
	//!
	//! *** WARNING ***
	//! 'DescriptorBinding::arrayCount' is *NOT* the same as 'VkDescriptorSetLayoutBinding::descriptorCount'.
	//!
	//! NOTE: D3D12 only supports shader visibility for a single individual stages or all stages so this
	//!       means that shader visibility can't be a combination of stage bits like Vulkan.
	//!       See: https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shader_visibility
	//!
	struct DescriptorBinding
	{
		uint32_t                binding = VALUE_IGNORED;               //
		grfx::DescriptorType    type = grfx::DESCRIPTOR_TYPE_UNDEFINED; //
		uint32_t                arrayCount = 1;                               // WARNING: Not VkDescriptorSetLayoutBinding::descriptorCount
		grfx::ShaderStageBits   shaderVisiblity = grfx::SHADER_STAGE_ALL;          // Single value not set of flags (see note above)
		DescriptorBindingFlags  flags;
		std::vector<SamplerPtr> immutableSamplers;

		DescriptorBinding() {}

		DescriptorBinding(
			uint32_t               binding_,
			grfx::DescriptorType   type_,
			uint32_t               arrayCount_ = 1,
			grfx::ShaderStageBits  shaderVisiblity_ = grfx::SHADER_STAGE_ALL,
			DescriptorBindingFlags flags_ = 0)
			: binding(binding_),
			type(type_),
			arrayCount(arrayCount_),
			shaderVisiblity(shaderVisiblity_),
			flags(flags_) {}
	};

	struct WriteDescriptor
	{
		uint32_t               binding = VALUE_IGNORED;
		uint32_t               arrayIndex = 0;
		grfx::DescriptorType   type = grfx::DESCRIPTOR_TYPE_UNDEFINED;
		uint64_t               bufferOffset = 0;
		uint64_t               bufferRange = 0;
		uint32_t               structuredElementCount = 0;
		const grfx::Buffer* pBuffer = nullptr;
		const grfx::ImageView* pImageView = nullptr;
		const grfx::Sampler* pSampler = nullptr;
	};

	// -------------------------------------------------------------------------------------------------

	//! @struct DescriptorPoolCreateInfo
	//!
	//!
	struct DescriptorPoolCreateInfo
	{
		uint32_t sampler = 0;
		uint32_t combinedImageSampler = 0;
		uint32_t sampledImage = 0;
		uint32_t storageImage = 0;
		uint32_t uniformTexelBuffer = 0;
		uint32_t storageTexelBuffer = 0;
		uint32_t uniformBuffer = 0;
		uint32_t rawStorageBuffer = 0;
		uint32_t structuredBuffer = 0;
		uint32_t uniformBufferDynamic = 0;
		uint32_t storageBufferDynamic = 0;
		uint32_t inputAttachment = 0;
	};

	//! @class DescriptorPool
	//!
	//!
	class DescriptorPool
		: public grfx::DeviceObject<grfx::DescriptorPoolCreateInfo>
	{
	public:
		DescriptorPool() {}
		virtual ~DescriptorPool() {}
	};

	// -------------------------------------------------------------------------------------------------

	namespace internal {

		//! @struct DescriptorSetCreateInfo
		//!
		//!
		struct DescriptorSetCreateInfo
		{
			grfx::DescriptorPool* pPool = nullptr;
			const grfx::DescriptorSetLayout* pLayout = nullptr;
		};

	} // namespace internal

	//! @class DescriptorSet
	//!
	//!
	class DescriptorSet
		: public grfx::DeviceObject<grfx::internal::DescriptorSetCreateInfo>
	{
	public:
		DescriptorSet() {}
		virtual ~DescriptorSet() {}

		grfx::DescriptorPoolPtr          GetPool() const { return mCreateInfo.pPool; }
		const grfx::DescriptorSetLayout* GetLayout() const { return mCreateInfo.pLayout; }

		virtual Result UpdateDescriptors(uint32_t writeCount, const grfx::WriteDescriptor* pWrites) = 0;

		Result UpdateSampler(
			uint32_t             binding,
			uint32_t             arrayIndex,
			const grfx::Sampler* pSampler);

		Result UpdateSampledImage(
			uint32_t                      binding,
			uint32_t                      arrayIndex,
			const grfx::SampledImageView* pImageView);

		Result UpdateSampledImage(
			uint32_t             binding,
			uint32_t             arrayIndex,
			const grfx::Texture* pTexture);

		Result UpdateStorageImage(
			uint32_t             binding,
			uint32_t             arrayIndex,
			const grfx::Texture* pTexture);

		Result UpdateUniformBuffer(
			uint32_t            binding,
			uint32_t            arrayIndex,
			const grfx::Buffer* pBuffer,
			uint64_t            offset = 0,
			uint64_t            range = WHOLE_SIZE);
	};

	// -------------------------------------------------------------------------------------------------

	//! @struct DescriptorSetLayoutCreateInfo
	//!
	//!
	struct DescriptorSetLayoutCreateInfo
	{
		grfx::DescriptorSetLayoutFlags       flags;
		std::vector<grfx::DescriptorBinding> bindings;
	};

	//! @class DescriptorSetLayout
	//!
	//!
	class DescriptorSetLayout
		: public grfx::DeviceObject<grfx::DescriptorSetLayoutCreateInfo>
	{
	public:
		DescriptorSetLayout() {}
		virtual ~DescriptorSetLayout() {}

		bool IsPushable() const { return mCreateInfo.flags.bits.pushable; }

		const std::vector<grfx::DescriptorBinding>& GetBindings() const { return mCreateInfo.bindings; }

	protected:
		virtual Result Create(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo) override;
		friend class grfx::Device;
	};

} // namespace grfx

#pragma endregion

#pragma region Shader

namespace grfx {

	struct ShaderModuleCreateInfo
	{
		uint32_t    size = 0;
		const char* pCode = nullptr;
	};

	class ShaderModule
		: public grfx::DeviceObject<grfx::ShaderModuleCreateInfo>
	{
	public:
		ShaderModule() {}
		virtual ~ShaderModule() {}
	};

} // namespace grfx

#pragma endregion

#pragma region ShadingRate

namespace grfx {

	// SupportedShadingRate
	//
	// A supported shading rate supported by the device.
	struct SupportedShadingRate
	{
		// Bit mask of supported sample counts.
		uint32_t sampleCountMask;

		// Size of the shading rate fragment size.
		Extent2D fragmentSize;
	};

	// ShadingRateCapabilities
	//
	// Information about GPU support for shading rate features.
	struct ShadingRateCapabilities
	{
		// The shading rate mode supported by this device.
		ShadingRateMode supportedShadingRateMode = SHADING_RATE_NONE;

		struct
		{
			bool supportsNonSubsampledImages;

			// Minimum/maximum size of the region of the render target
			// corresponding to a single pixel in the FDM attachment.
			// This is *not* the minimum/maximum fragment density.
			Extent2D minTexelSize;
			Extent2D maxTexelSize;
		} fdm;

		struct
		{
			// Minimum/maximum size of the region of the render target
			// corresponding to a single pixel in the VRS attachment.
			// This is *not* the shading rate itself.
			Extent2D minTexelSize;
			Extent2D maxTexelSize;

			// List of supported shading rates.
			std::vector<SupportedShadingRate> supportedRates;
		} vrs;
	};

	// ShadingRateEncoder
	//
	// Encodes fragment densities/sizes into the format needed for a
	// ShadingRatePattern.
	class ShadingRateEncoder
	{
	public:
		virtual ~ShadingRateEncoder() = default;

		// Encode a pair of fragment density values.
		//
		// Fragment density values are a ratio over 255, e.g. 255 means shade every
		// pixel, and 128 means shade every other pixel.
		virtual uint32_t EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const = 0;

		// Encode a pair of fragment size values.
		//
		// The fragmentWidth/fragmentHeight values are in pixels.
		virtual uint32_t EncodeFragmentSize(uint8_t fragmentWidth, uint8_t fragmentHeight) const = 0;
	};

	// ShadingRatePatternCreateInfo
	//
	//
	struct ShadingRatePatternCreateInfo
	{
		// The size of the framebuffer image that will be used with the created
		// ShadingRatePattern.
		Extent2D framebufferSize;

		// The size of the region of the framebuffer image that will correspond to
		// a single pixel in the ShadingRatePattern image.
		Extent2D texelSize;

		// The shading rate mode (FDM or VRS).
		ShadingRateMode shadingRateMode = SHADING_RATE_NONE;

		// The sample count of the render targets using this shading rate pattern.
		SampleCount sampleCount;
	};

	// ShadingRatePattern
	//
	// An image representing fragment sizes/densities that can be used in a render
	// pass to control the shading rate.
	class ShadingRatePattern
		: public DeviceObject<ShadingRatePatternCreateInfo>
	{
	public:
		virtual ~ShadingRatePattern() = default;

		// The shading rate mode (FDM or VRS).
		ShadingRateMode GetShadingRateMode() const { return mShadingRateMode; }

		// The image contaning encoded fragment sizes/densities.
		ImagePtr GetAttachmentImage() const { return mAttachmentImage; }

		// The width/height of the image contaning encoded fragment sizes/densities.
		uint32_t GetAttachmentWidth() const { return mAttachmentImage->GetWidth(); }
		uint32_t GetAttachmentHeight() const { return mAttachmentImage->GetHeight(); }

		// The width/height of the region of the render target image corresponding
		// to a single pixel in the image containing fragment sizes/densities.
		uint32_t GetTexelWidth() const { return mTexelSize.width; }
		uint32_t GetTexelHeight() const { return mTexelSize.height; }

		// The sample count of the render targets using this shading rate pattern.
		SampleCount GetSampleCount() const { return mSampleCount; }

		// Create a bitmap suitable for uploading fragment density/size to this pattern.
		std::unique_ptr<Bitmap> CreateBitmap() const;

		// Load fragment density/size from a bitmap of encoded values.
		Result LoadFromBitmap(Bitmap* bitmap);

		// Get the pixel format of a bitmap that can store the fragment density/size data.
		virtual Bitmap::Format GetBitmapFormat() const = 0;

		// Get an encoder that can encode fragment density/size values for this pattern.
		virtual const ShadingRateEncoder* GetShadingRateEncoder() const = 0;

	protected:
		ShadingRateMode mShadingRateMode;
		ImagePtr        mAttachmentImage;
		Extent2D        mTexelSize;
		SampleCount     mSampleCount;
	};

} // namespace grfx

#pragma endregion

#pragma region Shading Rate Utils

namespace grfx {
	// These set of functions create a bitmap.The bitmap can be used to produce
	// a ShadingRatePattern that in tun can be used in a render pass creation to
	// specify a non - uniform shading pattern.

	// A uniform bitmap that has cells of fragmentWidth and fragementHeight size
	void FillShadingRateUniformFragmentSize(ShadingRatePatternPtr pattern, uint32_t fragmentWidth, uint32_t fragmentHeight, Bitmap* bitmap);
	// A uniform bit map that has cells of xDensity and yDensity size
	void FillShadingRateUniformFragmentDensity(ShadingRatePatternPtr pattern, uint32_t xDensity, uint32_t yDensity, Bitmap* bitmap);
	// A map with cells of radial scale size
	void FillShadingRateRadial(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap);
	// A map with cells produced by anisotropic filter of scale size
	void FillShadingRateAnisotropic(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap);

} // namespace grfx

#pragma endregion

#pragma region Pipeline

namespace grfx {

	struct ShaderStageInfo
	{
		const grfx::ShaderModule* pModule = nullptr;
		std::string               entryPoint = "";
	};

	// -------------------------------------------------------------------------------------------------

	//! @struct ComputePipelineCreateInfo
	//!
	//!
	struct ComputePipelineCreateInfo
	{
		grfx::ShaderStageInfo          CS = {};
		const grfx::PipelineInterface* pPipelineInterface = nullptr;
	};

	//! @class ComputePipeline
	//!
	//!
	class ComputePipeline
		: public grfx::DeviceObject<grfx::ComputePipelineCreateInfo>
	{
	public:
		ComputePipeline() {}
		virtual ~ComputePipeline() {}

	protected:
		virtual Result Create(const grfx::ComputePipelineCreateInfo* pCreateInfo) override;
		friend class grfx::Device;
	};

	// -------------------------------------------------------------------------------------------------

	struct VertexInputState
	{
		uint32_t            bindingCount = 0;
		grfx::VertexBinding bindings[MAX_VERTEX_BINDINGS] = {};
	};

	struct InputAssemblyState
	{
		grfx::PrimitiveTopology topology = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool                    primitiveRestartEnable = false;
	};

	struct TessellationState
	{
		uint32_t                       patchControlPoints = 0;
		grfx::TessellationDomainOrigin domainOrigin = grfx::TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;
	};

	struct RasterState
	{
		bool              depthClampEnable = false;
		bool              rasterizeDiscardEnable = false;
		grfx::PolygonMode polygonMode = grfx::POLYGON_MODE_FILL;
		grfx::CullMode    cullMode = grfx::CULL_MODE_NONE;
		grfx::FrontFace   frontFace = grfx::FRONT_FACE_CCW;
		bool              depthBiasEnable = false;
		float             depthBiasConstantFactor = 0.0f;
		float             depthBiasClamp = 0.0f;
		float             depthBiasSlopeFactor = 0.0f;
		bool              depthClipEnable = false;
		grfx::SampleCount rasterizationSamples = grfx::SAMPLE_COUNT_1;
	};

	struct MultisampleState
	{
		bool alphaToCoverageEnable = false;
	};

	struct StencilOpState
	{
		grfx::StencilOp failOp = grfx::STENCIL_OP_KEEP;
		grfx::StencilOp passOp = grfx::STENCIL_OP_KEEP;
		grfx::StencilOp depthFailOp = grfx::STENCIL_OP_KEEP;
		grfx::CompareOp compareOp = grfx::COMPARE_OP_NEVER;
		uint32_t        compareMask = 0;
		uint32_t        writeMask = 0;
		uint32_t        reference = 0;
	};

	struct DepthStencilState
	{
		bool                 depthTestEnable = true;
		bool                 depthWriteEnable = true;
		grfx::CompareOp      depthCompareOp = grfx::COMPARE_OP_LESS;
		bool                 depthBoundsTestEnable = false;
		float                minDepthBounds = 0.0f;
		float                maxDepthBounds = 1.0f;
		bool                 stencilTestEnable = false;
		grfx::StencilOpState front = {};
		grfx::StencilOpState back = {};
	};

	struct BlendAttachmentState
	{
		bool                      blendEnable = false;
		grfx::BlendFactor         srcColorBlendFactor = grfx::BLEND_FACTOR_ONE;
		grfx::BlendFactor         dstColorBlendFactor = grfx::BLEND_FACTOR_ZERO;
		grfx::BlendOp             colorBlendOp = grfx::BLEND_OP_ADD;
		grfx::BlendFactor         srcAlphaBlendFactor = grfx::BLEND_FACTOR_ONE;
		grfx::BlendFactor         dstAlphaBlendFactor = grfx::BLEND_FACTOR_ZERO;
		grfx::BlendOp             alphaBlendOp = grfx::BLEND_OP_ADD;
		grfx::ColorComponentFlags colorWriteMask = grfx::ColorComponentFlags::RGBA();

		// These are best guesses based on random formulas off of the internet.
		// Correct later when authorative literature is found.
		//
		static grfx::BlendAttachmentState BlendModeAdditive();
		static grfx::BlendAttachmentState BlendModeAlpha();
		static grfx::BlendAttachmentState BlendModeOver();
		static grfx::BlendAttachmentState BlendModeUnder();
		static grfx::BlendAttachmentState BlendModePremultAlpha();
	};

	struct ColorBlendState
	{
		bool                       logicOpEnable = false;
		grfx::LogicOp              logicOp = grfx::LOGIC_OP_CLEAR;
		uint32_t                   blendAttachmentCount = 0;
		grfx::BlendAttachmentState blendAttachments[MAX_RENDER_TARGETS] = {};
		float                      blendConstants[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct OutputState
	{
		uint32_t     renderTargetCount = 0;
		grfx::Format renderTargetFormats[MAX_RENDER_TARGETS] = { grfx::FORMAT_UNDEFINED };
		grfx::Format depthStencilFormat = grfx::FORMAT_UNDEFINED;
	};

	//! @struct GraphicsPipelineCreateInfo
	//!
	//!
	struct GraphicsPipelineCreateInfo
	{
		grfx::ShaderStageInfo          VS = {};
		grfx::ShaderStageInfo          HS = {};
		grfx::ShaderStageInfo          DS = {};
		grfx::ShaderStageInfo          GS = {};
		grfx::ShaderStageInfo          PS = {};
		grfx::VertexInputState         vertexInputState = {};
		grfx::InputAssemblyState       inputAssemblyState = {};
		grfx::TessellationState        tessellationState = {};
		grfx::RasterState              rasterState = {};
		grfx::MultisampleState         multisampleState = {};
		grfx::DepthStencilState        depthStencilState = {};
		grfx::ColorBlendState          colorBlendState = {};
		grfx::OutputState              outputState = {};
		grfx::ShadingRateMode          shadingRateMode = grfx::SHADING_RATE_NONE;
		grfx::MultiViewState           multiViewState = {};
		const grfx::PipelineInterface* pPipelineInterface = nullptr;
		bool                           dynamicRenderPass = false;
	};

	struct GraphicsPipelineCreateInfo2
	{
		grfx::ShaderStageInfo          VS = {};
		grfx::ShaderStageInfo          PS = {};
		grfx::VertexInputState         vertexInputState = {};
		grfx::PrimitiveTopology        topology = grfx::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		grfx::PolygonMode              polygonMode = grfx::POLYGON_MODE_FILL;
		grfx::CullMode                 cullMode = grfx::CULL_MODE_NONE;
		grfx::FrontFace                frontFace = grfx::FRONT_FACE_CCW;
		bool                           depthReadEnable = true;
		bool                           depthWriteEnable = true;
		grfx::CompareOp                depthCompareOp = grfx::COMPARE_OP_LESS;
		grfx::BlendMode                blendModes[MAX_RENDER_TARGETS] = { grfx::BLEND_MODE_NONE };
		grfx::OutputState              outputState = {};
		grfx::ShadingRateMode          shadingRateMode = grfx::SHADING_RATE_NONE;
		grfx::MultiViewState           multiViewState = {};
		const grfx::PipelineInterface* pPipelineInterface = nullptr;
		bool                           dynamicRenderPass = false;
	};

	namespace internal {

		void FillOutGraphicsPipelineCreateInfo(
			const grfx::GraphicsPipelineCreateInfo2* pSrcCreateInfo,
			grfx::GraphicsPipelineCreateInfo* pDstCreateInfo);

	} // namespace internal

	//! @class GraphicsPipeline
	//!
	//!
	class GraphicsPipeline
		: public grfx::DeviceObject<grfx::GraphicsPipelineCreateInfo>
	{
	public:
		GraphicsPipeline() {}
		virtual ~GraphicsPipeline() {}

	protected:
		virtual Result Create(const grfx::GraphicsPipelineCreateInfo* pCreateInfo) override;
		friend class grfx::Device;
	};

	// -------------------------------------------------------------------------------------------------

	//! @struct PipelineInterfaceCreateInfo
	//!
	//!
	struct PipelineInterfaceCreateInfo
	{
		uint32_t setCount = 0;
		struct
		{
			uint32_t                         set = VALUE_IGNORED; // Set number
			const grfx::DescriptorSetLayout* pLayout = nullptr;           // Set layout
		} sets[MAX_BOUND_DESCRIPTOR_SETS] = {};

		// VK: Push constants
		// DX: Root constants
		//
		// Push/root constants are measured in DWORDs (uint32_t) aka 32-bit values.
		//
		// The binding and set for push constants CAN NOT overlap with a binding
		// AND set in sets (the struct immediately above this one). It's okay for
		// push constants to be in an existing set at binding that is not used
		// by an entry in the set layout.
		//
		struct
		{
			uint32_t              count = 0;                 // Measured in DWORDs, must be less than or equal to MAX_PUSH_CONSTANTS
			uint32_t              binding = VALUE_IGNORED; // D3D12 only, ignored by Vulkan
			uint32_t              set = VALUE_IGNORED; // D3D12 only, ignored by Vulkan
			grfx::ShaderStageBits shaderVisiblity = grfx::SHADER_STAGE_ALL;
		} pushConstants;
	};

	//! @class PipelineInterface
	//!
	//! VK: Pipeline layout
	//! DX: Root signature
	//!
	class PipelineInterface
		: public grfx::DeviceObject<grfx::PipelineInterfaceCreateInfo>
	{
	public:
		PipelineInterface() {}
		virtual ~PipelineInterface() {}

		bool                         HasConsecutiveSetNumbers() const { return mHasConsecutiveSetNumbers; }
		const std::vector<uint32_t>& GetSetNumbers() const { return mSetNumbers; }

		const grfx::DescriptorSetLayout* GetSetLayout(uint32_t setNumber) const;

	protected:
		virtual Result Create(const grfx::PipelineInterfaceCreateInfo* pCreateInfo) override;
		friend class grfx::Device;

	private:
		bool                  mHasConsecutiveSetNumbers = false;
		std::vector<uint32_t> mSetNumbers = {};
	};

} // namespace grfx

#pragma endregion

#pragma region DrawPass

namespace grfx {

	struct RenderPassBeginInfo;

	//! @struct DrawPassCreateInfo
	//!
	//! Use this version if the format(s) are known but images need creation.
	//!
	//! Backing images will be created using the criteria provided in this struct.
	//!
	struct DrawPassCreateInfo
	{
		uint32_t                     width = 0;
		uint32_t                     height = 0;
		grfx::SampleCount            sampleCount = grfx::SAMPLE_COUNT_1;
		uint32_t                     renderTargetCount = 0;
		grfx::Format                 renderTargetFormats[MAX_RENDER_TARGETS] = {};
		grfx::Format                 depthStencilFormat = grfx::FORMAT_UNDEFINED;
		grfx::ImageUsageFlags        renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
		grfx::ImageUsageFlags        depthStencilUsageFlags = {};
		grfx::ResourceState          renderTargetInitialStates[MAX_RENDER_TARGETS] = { grfx::RESOURCE_STATE_RENDER_TARGET };
		grfx::ResourceState          depthStencilInitialState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		grfx::RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
		grfx::DepthStencilClearValue depthStencilClearValue = {};
		grfx::ShadingRatePattern* pShadingRatePattern = nullptr;
		grfx::ImageCreateFlags       imageCreateFlags = {};
	};

	//! @struct DrawPassCreateInfo2
	//!
	//! Use this version if the images exists.
	//!
	struct DrawPassCreateInfo2
	{
		uint32_t                     width = 0;
		uint32_t                     height = 0;
		uint32_t                     renderTargetCount = 0;
		grfx::Image* pRenderTargetImages[MAX_RENDER_TARGETS] = {};
		grfx::Image* pDepthStencilImage = nullptr;
		grfx::ResourceState          depthStencilState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		grfx::RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
		grfx::DepthStencilClearValue depthStencilClearValue = {};
		grfx::ShadingRatePattern* pShadingRatePattern = nullptr;
	};

	//! struct DrawPassCreateInfo3
	//!
	//! Use this version if the textures exists.
	//!
	struct DrawPassCreateInfo3
	{
		uint32_t                  width = 0;
		uint32_t                  height = 0;
		uint32_t                  renderTargetCount = 0;
		grfx::Texture* pRenderTargetTextures[MAX_RENDER_TARGETS] = {};
		grfx::Texture* pDepthStencilTexture = nullptr;
		grfx::ResourceState       depthStencilState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		grfx::ShadingRatePattern* pShadingRatePattern = nullptr;
	};

	namespace internal {

		struct DrawPassCreateInfo
		{
			enum CreateInfoVersion
			{
				CREATE_INFO_VERSION_UNDEFINED = 0,
				CREATE_INFO_VERSION_1 = 1,
				CREATE_INFO_VERSION_2 = 2,
				CREATE_INFO_VERSION_3 = 3,
			};

			CreateInfoVersion         version = CREATE_INFO_VERSION_UNDEFINED;
			uint32_t                  width = 0;
			uint32_t                  height = 0;
			uint32_t                  renderTargetCount = 0;
			grfx::ResourceState       depthStencilState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
			grfx::ShadingRatePattern* pShadingRatePattern = nullptr;

			// Data unique to grfx::DrawPassCreateInfo1
			struct
			{
				grfx::SampleCount      sampleCount = grfx::SAMPLE_COUNT_1;
				grfx::Format           renderTargetFormats[MAX_RENDER_TARGETS] = {};
				grfx::Format           depthStencilFormat = grfx::FORMAT_UNDEFINED;
				grfx::ImageUsageFlags  renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
				grfx::ImageUsageFlags  depthStencilUsageFlags = {};
				grfx::ResourceState    renderTargetInitialStates[MAX_RENDER_TARGETS] = { grfx::RESOURCE_STATE_RENDER_TARGET };
				grfx::ResourceState    depthStencilInitialState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
				grfx::ImageCreateFlags imageCreateFlags = {};
			} V1;

			// Data unique to grfx::DrawPassCreateInfo2
			struct
			{
				grfx::Image* pRenderTargetImages[MAX_RENDER_TARGETS] = {};
				grfx::Image* pDepthStencilImage = nullptr;
			} V2;

			// Data unique to grfx::DrawPassCreateInfo3
			struct
			{
				grfx::Texture* pRenderTargetTextures[MAX_RENDER_TARGETS] = {};
				grfx::Texture* pDepthStencilTexture = nullptr;
			} V3;

			// Clear values
			grfx::RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
			grfx::DepthStencilClearValue depthStencilClearValue = {};

			DrawPassCreateInfo() {}
			DrawPassCreateInfo(const grfx::DrawPassCreateInfo& obj);
			DrawPassCreateInfo(const grfx::DrawPassCreateInfo2& obj);
			DrawPassCreateInfo(const grfx::DrawPassCreateInfo3& obj);
		};

	} // namespace internal

	//! @class DrawPass
	//!
	//!
	class DrawPass
		: public DeviceObject<grfx::internal::DrawPassCreateInfo>
	{
	public:
		DrawPass() {}
		virtual ~DrawPass() {}

		uint32_t              GetWidth() const { return mCreateInfo.width; }
		uint32_t              GetHeight() const { return mCreateInfo.height; }
		const grfx::Rect& GetRenderArea() const;
		const grfx::Rect& GetScissor() const;
		const grfx::Viewport& GetViewport() const;

		uint32_t       GetRenderTargetCount() const { return mCreateInfo.renderTargetCount; }
		Result         GetRenderTargetTexture(uint32_t index, grfx::Texture** ppRenderTarget) const;
		grfx::Texture* GetRenderTargetTexture(uint32_t index) const;
		bool           HasDepthStencil() const { return mDepthStencilTexture ? true : false; }
		Result         GetDepthStencilTexture(grfx::Texture** ppDepthStencil) const;
		grfx::Texture* GetDepthStencilTexture() const;

		void PrepareRenderPassBeginInfo(const grfx::DrawPassClearFlags& clearFlags, grfx::RenderPassBeginInfo* pBeginInfo) const;

	protected:
		virtual Result CreateApiObjects(const grfx::internal::DrawPassCreateInfo* pCreateInfo) override;
		virtual void   DestroyApiObjects() override;

	private:
		Result CreateTexturesV1(const grfx::internal::DrawPassCreateInfo* pCreateInfo);
		Result CreateTexturesV2(const grfx::internal::DrawPassCreateInfo* pCreateInfo);
		Result CreateTexturesV3(const grfx::internal::DrawPassCreateInfo* pCreateInfo);

	private:
		grfx::Rect                    mRenderArea = {};
		std::vector<grfx::TexturePtr> mRenderTargetTextures;
		grfx::TexturePtr              mDepthStencilTexture;

		struct Pass
		{
			uint32_t            clearMask = 0;
			grfx::RenderPassPtr renderPass;
		};

		std::vector<Pass> mPasses;
	};

} // namespace grfx

#pragma endregion

#pragma region RenderPass

namespace grfx {

	//! @struct RenderPassCreateInfo
	//!
	//! Use this if the RTVs and/or the DSV exists.
	//!
	struct RenderPassCreateInfo
	{
		uint32_t                     width = 0;
		uint32_t                     height = 0;
		uint32_t                     arrayLayerCount = 1;
		uint32_t                     renderTargetCount = 0;
		grfx::MultiViewState         multiViewState = {};
		grfx::RenderTargetView* pRenderTargetViews[MAX_RENDER_TARGETS] = {};
		grfx::DepthStencilView* pDepthStencilView = nullptr;
		grfx::ResourceState          depthStencilState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		grfx::RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
		grfx::DepthStencilClearValue depthStencilClearValue = {};
		grfx::Ownership              ownership = grfx::OWNERSHIP_REFERENCE;

		// If `pShadingRatePattern` is not null, then the pipeline targeting this
		// RenderPass must use the same shading rate mode
		// (`GraphicsPipelineCreateInfo.shadingRateMode`).
		grfx::ShadingRatePatternPtr pShadingRatePattern = nullptr;

		void SetAllRenderTargetClearValue(const grfx::RenderTargetClearValue& value);
	};

	//! @struct RenderPassCreateInfo2
	//!
	//! Use this version if the format(s) are know but images and
	//! views need creation.
	//!
	//! RTVs, DSV, and backing images will be created using the
	//! criteria provided in this struct.
	//!
	struct RenderPassCreateInfo2
	{
		uint32_t                     width = 0;
		uint32_t                     height = 0;
		uint32_t                     arrayLayerCount = 1;
		grfx::MultiViewState         multiViewState = {};
		grfx::SampleCount            sampleCount = grfx::SAMPLE_COUNT_1;
		uint32_t                     renderTargetCount = 0;
		grfx::Format                 renderTargetFormats[MAX_RENDER_TARGETS] = {};
		grfx::Format                 depthStencilFormat = grfx::FORMAT_UNDEFINED;
		grfx::ImageUsageFlags        renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
		grfx::ImageUsageFlags        depthStencilUsageFlags = {};
		grfx::RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
		grfx::DepthStencilClearValue depthStencilClearValue = {};
		grfx::AttachmentLoadOp       renderTargetLoadOps[MAX_RENDER_TARGETS] = { grfx::ATTACHMENT_LOAD_OP_LOAD };
		grfx::AttachmentStoreOp      renderTargetStoreOps[MAX_RENDER_TARGETS] = { grfx::ATTACHMENT_STORE_OP_STORE };
		grfx::AttachmentLoadOp       depthLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
		grfx::AttachmentStoreOp      depthStoreOp = grfx::ATTACHMENT_STORE_OP_STORE;
		grfx::AttachmentLoadOp       stencilLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
		grfx::AttachmentStoreOp      stencilStoreOp = grfx::ATTACHMENT_STORE_OP_STORE;
		grfx::ResourceState          renderTargetInitialStates[MAX_RENDER_TARGETS] = { grfx::RESOURCE_STATE_UNDEFINED };
		grfx::ResourceState          depthStencilInitialState = grfx::RESOURCE_STATE_UNDEFINED;
		grfx::Ownership              ownership = grfx::OWNERSHIP_REFERENCE;

		// If `pShadingRatePattern` is not null, then the pipeline targeting this
		// RenderPass must use the same shading rate mode
		// (`GraphicsPipelineCreateInfo.shadingRateMode`).
		grfx::ShadingRatePatternPtr pShadingRatePattern = nullptr;

		void SetAllRenderTargetUsageFlags(const grfx::ImageUsageFlags& flags);
		void SetAllRenderTargetClearValue(const grfx::RenderTargetClearValue& value);
		void SetAllRenderTargetLoadOp(grfx::AttachmentLoadOp op);
		void SetAllRenderTargetStoreOp(grfx::AttachmentStoreOp op);
		void SetAllRenderTargetToClear();
	};

	//! @struct RenderPassCreateInfo3
	//!
	//! Use this if the images exists but views need creation.
	//!
	struct RenderPassCreateInfo3
	{
		uint32_t                     width = 0;
		uint32_t                     height = 0;
		uint32_t                     renderTargetCount = 0;
		uint32_t                     arrayLayerCount = 1;
		grfx::MultiViewState         multiViewState = {};
		grfx::Image* pRenderTargetImages[MAX_RENDER_TARGETS] = {};
		grfx::Image* pDepthStencilImage = nullptr;
		grfx::ResourceState          depthStencilState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		grfx::RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
		grfx::DepthStencilClearValue depthStencilClearValue = {};
		grfx::AttachmentLoadOp       renderTargetLoadOps[MAX_RENDER_TARGETS] = { grfx::ATTACHMENT_LOAD_OP_LOAD };
		grfx::AttachmentStoreOp      renderTargetStoreOps[MAX_RENDER_TARGETS] = { grfx::ATTACHMENT_STORE_OP_STORE };
		grfx::AttachmentLoadOp       depthLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
		grfx::AttachmentStoreOp      depthStoreOp = grfx::ATTACHMENT_STORE_OP_STORE;
		grfx::AttachmentLoadOp       stencilLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
		grfx::AttachmentStoreOp      stencilStoreOp = grfx::ATTACHMENT_STORE_OP_STORE;
		grfx::Ownership              ownership = grfx::OWNERSHIP_REFERENCE;

		// If `pShadingRatePattern` is not null, then the pipeline targeting this
		// RenderPass must use the same shading rate mode
		// (`GraphicsPipelineCreateInfo.shadingRateMode`).
		grfx::ShadingRatePatternPtr pShadingRatePattern = nullptr;

		void SetAllRenderTargetClearValue(const grfx::RenderTargetClearValue& value);
		void SetAllRenderTargetLoadOp(grfx::AttachmentLoadOp op);
		void SetAllRenderTargetStoreOp(grfx::AttachmentStoreOp op);
		void SetAllRenderTargetToClear();
	};

	namespace internal {

		struct RenderPassCreateInfo
		{
			enum CreateInfoVersion
			{
				CREATE_INFO_VERSION_UNDEFINED = 0,
				CREATE_INFO_VERSION_1 = 1,
				CREATE_INFO_VERSION_2 = 2,
				CREATE_INFO_VERSION_3 = 3,
			};

			grfx::Ownership           ownership = grfx::OWNERSHIP_REFERENCE;
			CreateInfoVersion         version = CREATE_INFO_VERSION_UNDEFINED;
			uint32_t                  width = 0;
			uint32_t                  height = 0;
			uint32_t                  renderTargetCount = 0;
			uint32_t                  arrayLayerCount = 1;
			grfx::MultiViewState      multiViewState = {};
			grfx::ResourceState       depthStencilState = grfx::RESOURCE_STATE_DEPTH_STENCIL_WRITE;
			grfx::ShadingRatePattern* pShadingRatePattern = nullptr;

			// Data unique to grfx::RenderPassCreateInfo
			struct
			{
				grfx::RenderTargetView* pRenderTargetViews[MAX_RENDER_TARGETS] = {};
				grfx::DepthStencilView* pDepthStencilView = nullptr;
			} V1;

			// Data unique to grfx::RenderPassCreateInfo2
			struct
			{
				grfx::SampleCount     sampleCount = grfx::SAMPLE_COUNT_1;
				grfx::Format          renderTargetFormats[MAX_RENDER_TARGETS] = {};
				grfx::Format          depthStencilFormat = grfx::FORMAT_UNDEFINED;
				grfx::ImageUsageFlags renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
				grfx::ImageUsageFlags depthStencilUsageFlags = {};
				grfx::ResourceState   renderTargetInitialStates[MAX_RENDER_TARGETS] = { grfx::RESOURCE_STATE_UNDEFINED };
				grfx::ResourceState   depthStencilInitialState = grfx::RESOURCE_STATE_UNDEFINED;
			} V2;

			// Data unique to grfx::RenderPassCreateInfo3
			struct
			{
				grfx::Image* pRenderTargetImages[MAX_RENDER_TARGETS] = {};
				grfx::Image* pDepthStencilImage = nullptr;
			} V3;

			// Clear values
			grfx::RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
			grfx::DepthStencilClearValue depthStencilClearValue = {};

			// Load/store ops
			grfx::AttachmentLoadOp  renderTargetLoadOps[MAX_RENDER_TARGETS] = { grfx::ATTACHMENT_LOAD_OP_LOAD };
			grfx::AttachmentStoreOp renderTargetStoreOps[MAX_RENDER_TARGETS] = { grfx::ATTACHMENT_STORE_OP_STORE };
			grfx::AttachmentLoadOp  depthLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
			grfx::AttachmentStoreOp depthStoreOp = grfx::ATTACHMENT_STORE_OP_STORE;
			grfx::AttachmentLoadOp  stencilLoadOp = grfx::ATTACHMENT_LOAD_OP_LOAD;
			grfx::AttachmentStoreOp stencilStoreOp = grfx::ATTACHMENT_STORE_OP_STORE;

			RenderPassCreateInfo() {}
			RenderPassCreateInfo(const grfx::RenderPassCreateInfo& obj);
			RenderPassCreateInfo(const grfx::RenderPassCreateInfo2& obj);
			RenderPassCreateInfo(const grfx::RenderPassCreateInfo3& obj);
		};

	} // namespace internal

	//! @class RenderPass
	//!
	//!
	class RenderPass
		: public grfx::DeviceObject<grfx::internal::RenderPassCreateInfo>
	{
	public:
		RenderPass() {}
		virtual ~RenderPass() {}

		const grfx::Rect& GetRenderArea() const { return mRenderArea; }
		const grfx::Rect& GetScissor() const { return mRenderArea; }
		const grfx::Viewport& GetViewport() const { return mViewport; }

		uint32_t GetRenderTargetCount() const { return mCreateInfo.renderTargetCount; }
		bool     HasDepthStencil() const { return mDepthStencilImage ? true : false; }

		Result GetRenderTargetView(uint32_t index, grfx::RenderTargetView** ppView) const;
		Result GetDepthStencilView(grfx::DepthStencilView** ppView) const;

		Result GetRenderTargetImage(uint32_t index, grfx::Image** ppImage) const;
		Result GetDepthStencilImage(grfx::Image** ppImage) const;

		//! This only applies to grfx::RenderPass objects created using grfx::RenderPassCreateInfo2.
		//! These functions will set 'isExternal' to true resulting in these objects NOT getting
		//! destroyed when the encapsulating grfx::RenderPass object is destroyed.
		//!
		//! Calling these fuctions on grfx::RenderPass objects created using using grfx::RenderPassCreateInfo
		//! will still return a valid object if the index or DSV object exists.
		//!
		Result DisownRenderTargetView(uint32_t index, grfx::RenderTargetView** ppView);
		Result DisownDepthStencilView(grfx::DepthStencilView** ppView);
		Result DisownRenderTargetImage(uint32_t index, grfx::Image** ppImage);
		Result DisownDepthStencilImage(grfx::Image** ppImage);

		// Convenience functions returns empty ptr if index is out of range or DSV object does not exist.
		grfx::RenderTargetViewPtr GetRenderTargetView(uint32_t index) const;
		grfx::DepthStencilViewPtr GetDepthStencilView() const;
		grfx::ImagePtr            GetRenderTargetImage(uint32_t index) const;
		grfx::ImagePtr            GetDepthStencilImage() const;

		// Returns index of pImage otherwise returns UINT32_MAX
		uint32_t GetRenderTargetImageIndex(const grfx::Image* pImage) const;

		// Returns true if render targets or depth stencil contains ATTACHMENT_LOAD_OP_CLEAR
		bool HasLoadOpClear() const { return mHasLoadOpClear; }

	protected:
		virtual Result Create(const grfx::internal::RenderPassCreateInfo* pCreateInfo) override;
		virtual void   Destroy() override;
		friend class grfx::Device;

	private:
		Result CreateImagesAndViewsV1(const grfx::internal::RenderPassCreateInfo* pCreateInfo);
		Result CreateImagesAndViewsV2(const grfx::internal::RenderPassCreateInfo* pCreateInfo);
		Result CreateImagesAndViewsV3(const grfx::internal::RenderPassCreateInfo* pCreateInfo);

	protected:
		grfx::Rect                             mRenderArea = {};
		grfx::Viewport                         mViewport = {};
		std::vector<grfx::RenderTargetViewPtr> mRenderTargetViews;
		grfx::DepthStencilViewPtr              mDepthStencilView;
		std::vector<grfx::ImagePtr>            mRenderTargetImages;
		grfx::ImagePtr                         mDepthStencilImage;
		bool                                   mHasLoadOpClear = false;
	};

} // namespace grfx

#pragma endregion

#pragma region Command


// *** Graphics API Note ***
//
// In the cosmos of game engines, there's more than one way to build
// command buffers and track various bits that company that.
//
// Smaller engines and graphics demo may favor command buffer reuse or
// at least reusing the same resources in a similar order per frame.
//
// Larger engines may have have an entire job system where available
// command buffers are use for the next immediate task. There may
// or may not be any affinity for command buffers and tasks.
//
// We're going to favor the second case - command buffers do not
// have affinity for tasks. This means that for D3D12 we'll copy
// descriptors from the set's CPU heaps to the command buffer's
// GPU visible heaps when the BindGraphicsDescriptorSets or
// BindComputeDescriptorSets is called. This may not be the most
// efficient way to do this but it does give us the flexiblity
// to shape D3D12 to look like Vulkan.
//

namespace grfx {

	//! @struct BufferToBufferCopyInfo
	//!
	//!
	struct BufferToBufferCopyInfo
	{
		uint64_t size = 0;

		struct srcBuffer
		{
			uint64_t offset = 0;
		} srcBuffer;

		struct
		{
			uint32_t offset = 0;
		} dstBuffer;
	};

	//! @struct BufferToImageCopyInfo
	//!
	//!
	struct BufferToImageCopyInfo
	{
		struct
		{
			uint32_t imageWidth = 0; // [pixels]
			uint32_t imageHeight = 0; // [pixels]
			uint32_t imageRowStride = 0; // [bytes]
			uint64_t footprintOffset = 0; // [bytes]
			uint32_t footprintWidth = 0; // [pixels]
			uint32_t footprintHeight = 0; // [pixels]
			uint32_t footprintDepth = 0; // [pixels]
		} srcBuffer;

		struct
		{
			uint32_t mipLevel = 0;
			uint32_t arrayLayer = 0; // Must be 0 for 3D images
			uint32_t arrayLayerCount = 0; // Must be 1 for 3D images
			uint32_t x = 0; // [pixels]
			uint32_t y = 0; // [pixels]
			uint32_t z = 0; // [pixels]
			uint32_t width = 0; // [pixels]
			uint32_t height = 0; // [pixels]
			uint32_t depth = 0; // [pixels]
		} dstImage;
	};

	//! @struct ImageToBufferCopyInfo
	//!
	//!
	struct ImageToBufferCopyInfo
	{
		struct
		{
			uint32_t mipLevel = 0;
			uint32_t arrayLayer = 0; // Must be 0 for 3D images
			uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
			struct
			{
				uint32_t x = 0; // [pixels]
				uint32_t y = 0; // [pixels]
				uint32_t z = 0; // [pixels]
			} offset;
		} srcImage;

		struct
		{
			uint32_t x = 0; // [pixels]
			uint32_t y = 0; // [pixels]
			uint32_t z = 0; // [pixels]
		} extent;
	};

	//! @struct ImageToBufferOutputPitch
	//!
	//!
	struct ImageToBufferOutputPitch
	{
		uint32_t rowPitch = 0;
	};

	//! @struct ImageToImageCopyInfo
	//!
	//!
	struct ImageToImageCopyInfo
	{
		struct
		{
			uint32_t mipLevel = 0;
			uint32_t arrayLayer = 0; // Must be 0 for 3D images
			uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
			struct
			{
				uint32_t x = 0; // [pixels]
				uint32_t y = 0; // [pixels]
				uint32_t z = 0; // [pixels]
			} offset;
		} srcImage;

		struct
		{
			uint32_t mipLevel = 0;
			uint32_t arrayLayer = 0; // Must be 0 for 3D images
			uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
			struct
			{
				uint32_t x = 0; // [pixels]
				uint32_t y = 0; // [pixels]
				uint32_t z = 0; // [pixels]
			} offset;
		} dstImage;

		struct
		{
			uint32_t x = 0; // [pixels]
			uint32_t y = 0; // [pixels]
			uint32_t z = 0; // [pixels]
		} extent;
	};

	// struct ImageBlitInfo
	struct ImageBlitInfo
	{
		struct ImgInfo
		{
			uint32_t mipLevel = 0;
			uint32_t arrayLayer = 0; // Must be 0 for 3D images
			uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
			struct
			{
				uint32_t x = 0; // [pixels]
				uint32_t y = 0; // [pixels]
				uint32_t z = 0; // [pixels]
			} offsets[2];
		};

		ImgInfo srcImage;
		ImgInfo dstImage;

		Filter filter = FILTER_LINEAR;
	};

	// -------------------------------------------------------------------------------------------------

	struct RenderPassBeginInfo
	{
		//
		// The value of RTVClearCount cannot be less than the number
		// of RTVs in pRenderPass.
		//
		const grfx::RenderPass* pRenderPass = nullptr;
		grfx::Rect                   renderArea = {};
		uint32_t                     RTVClearCount = 0;
		grfx::RenderTargetClearValue RTVClearValues[MAX_RENDER_TARGETS] = { 0.0f, 0.0f, 0.0f, 0.0f };
		grfx::DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };
	};

	// RenderingInfo is used to start dynamic render passes.
	struct RenderingInfo
	{
		grfx::BeginRenderingFlags flags;
		grfx::Rect                renderArea = {};

		uint32_t                renderTargetCount = 0;
		grfx::RenderTargetView* pRenderTargetViews[MAX_RENDER_TARGETS] = {};
		grfx::DepthStencilView* pDepthStencilView = nullptr;

		grfx::RenderTargetClearValue RTVClearValues[MAX_RENDER_TARGETS] = { 0.0f, 0.0f, 0.0f, 0.0f };
		grfx::DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };
	};

	// -------------------------------------------------------------------------------------------------

	//! @struct CommandPoolCreateInfo
	//!
	//!
	struct CommandPoolCreateInfo
	{
		const grfx::Queue* pQueue = nullptr;
	};

	//! @class CommandPool
	//!
	//!
	class CommandPool
		: public grfx::DeviceObject<grfx::CommandPoolCreateInfo>
	{
	public:
		CommandPool() {}
		virtual ~CommandPool() {}

		grfx::CommandType GetCommandType() const;
	};

	// -------------------------------------------------------------------------------------------------

	namespace internal {

		//! @struct CommandBufferCreateInfo
		//!
		//! For D3D12 every command buffer will have two GPU visible descriptor heaps:
		//!   - one for CBVSRVUAV descriptors
		//!   - one for Sampler descriptors
		//!
		//! Both of heaps are set when the command buffer begins.
		//!
		//! Each time that BindGraphicsDescriptorSets or BindComputeDescriptorSets
		//! is called, the contents of each descriptor set's CBVSRVAUV and Sampler heaps
		//! will be copied into the command buffer's respective heap.
		//!
		//! The offsets from used in the copies will be saved and used to set the
		//! root descriptor tables.
		//!
		//! 'resourceDescriptorCount' and 'samplerDescriptorCount' tells the
		//! D3D12 command buffer how large CBVSRVUAV and Sampler heaps should be.
		//!
		//! 'samplerDescriptorCount' cannot exceed MAX_SAMPLER_DESCRIPTORS.
		//!
		//! Vulkan does not use 'samplerDescriptorCount' or 'samplerDescriptorCount'.
		//!
		struct CommandBufferCreateInfo
		{
			const grfx::CommandPool* pPool = nullptr;
			uint32_t                 resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT;
			uint32_t                 samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT;
		};

	} // namespace internal

	//! @class CommandBuffer
	//!
	//!
	class CommandBuffer
		: public grfx::DeviceObject<grfx::internal::CommandBufferCreateInfo>
	{
	public:
		CommandBuffer() {}
		virtual ~CommandBuffer() {}

		grfx::CommandType GetCommandType() const { return mCreateInfo.pPool->GetCommandType(); }

		virtual Result Begin() = 0;
		virtual Result End() = 0;

		void BeginRenderPass(const grfx::RenderPassBeginInfo* pBeginInfo);
		void EndRenderPass();

		void BeginRendering(const grfx::RenderingInfo* pRenderingInfo);
		void EndRendering();

		const grfx::RenderPass* GetCurrentRenderPass() const { return mCurrentRenderPass; }

		//
		// Clear functions must be called between BeginRenderPass and EndRenderPass.
		// Arg for pImage must be an image in the current render pass.
		//
		// TODO: add support for calling inside a dynamic render pass
		// (i.e., BeginRendering and EndRendering).
		virtual void ClearRenderTarget(
			grfx::Image* pImage,
			const grfx::RenderTargetClearValue& clearValue) = 0;
		virtual void ClearDepthStencil(
			grfx::Image* pImage,
			const grfx::DepthStencilClearValue& clearValue,
			uint32_t                            clearFlags) = 0;

		//! @fn TransitionImageLayout
		//!
		//! Vulkan requires a queue ownership transfer if a resource
		//! is used by queues in different queue families:
		//!  - Use \b pSrcQueue to specify a queue in the source queue family
		//!  - Use \b pDstQueue to specify a queue in the destination queue family
		//!  - If \b pSrcQueue and \b pDstQueue belong to the same queue family
		//!    then the queue ownership transfer won't happen.
		//!
		//! D3D12 ignores both \b pSrcQueue and \b pDstQueue since they're not
		//! relevant.
		//!
		virtual void TransitionImageLayout(
			const grfx::Image* pImage,
			uint32_t            mipLevel,
			uint32_t            mipLevelCount,
			uint32_t            arrayLayer,
			uint32_t            arrayLayerCount,
			grfx::ResourceState beforeState,
			grfx::ResourceState afterState,
			const grfx::Queue* pSrcQueue = nullptr,
			const grfx::Queue* pDstQueue = nullptr) = 0;

		//
		// See comment at function \b TransitionImageLayout for details
		// on queue ownership transfer.
		//
		virtual void BufferResourceBarrier(
			const grfx::Buffer* pBuffer,
			grfx::ResourceState beforeState,
			grfx::ResourceState afterState,
			const grfx::Queue* pSrcQueue = nullptr,
			const grfx::Queue* pDstQueue = nullptr) = 0;

		virtual void SetViewports(
			uint32_t              viewportCount,
			const grfx::Viewport* pViewports) = 0;

		virtual void SetScissors(
			uint32_t          scissorCount,
			const grfx::Rect* pScissors) = 0;

		virtual void BindGraphicsDescriptorSets(
			const grfx::PipelineInterface* pInterface,
			uint32_t                          setCount,
			const grfx::DescriptorSet* const* ppSets) = 0;

		//
		// Parameters count and dstOffset are measured in DWORDs (uint32_t) aka 32-bit values.
		// To set the first 4 32-bit values, use count = 4, dstOffset = 0.
		// To set the 16 DWORDs starting at offset 8, use count=16, dstOffset = 8.
		//
		// VK: pValues is subjected to Vulkan packing rules. BigWheels compiles HLSL
		//     shaders with -fvk-use-dx-layout on. This makes the packing rules match
		//     that of D3D12. However, if a shader is compiled without that flag or
		//     with a different compiler or source language. The contents pointed to
		//     by pValues must respect the packing rules in effect.
		//
		virtual void PushGraphicsConstants(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       count,
			const void* pValues,
			uint32_t                       dstOffset = 0) = 0;

		virtual void PushGraphicsUniformBuffer(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			uint32_t                       bufferOffset,
			const grfx::Buffer* pBuffer);

		virtual void PushGraphicsStructuredBuffer(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			uint32_t                       bufferOffset,
			const grfx::Buffer* pBuffer);

		virtual void PushGraphicsStorageBuffer(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			uint32_t                       bufferOffset,
			const grfx::Buffer* pBuffer);

		virtual void PushGraphicsSampledImage(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			const grfx::SampledImageView* pView);

		virtual void PushGraphicsStorageImage(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			const grfx::StorageImageView* pView);

		virtual void PushGraphicsSampler(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			const grfx::Sampler* pSampler);

		virtual void BindGraphicsPipeline(const grfx::GraphicsPipeline* pPipeline) = 0;

		virtual void BindComputeDescriptorSets(
			const grfx::PipelineInterface* pInterface,
			uint32_t                          setCount,
			const grfx::DescriptorSet* const* ppSets) = 0;

		// See comments at SetGraphicsPushConstants for explanation about count, pValues and dstOffset.
		virtual void PushComputeConstants(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       count,
			const void* pValues,
			uint32_t                       dstOffset = 0) = 0;

		virtual void PushComputeUniformBuffer(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			uint32_t                       bufferOffset,
			const grfx::Buffer* pBuffer);

		virtual void PushComputeStructuredBuffer(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			uint32_t                       bufferOffset,
			const grfx::Buffer* pBuffer);

		virtual void PushComputeStorageBuffer(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			uint32_t                       bufferOffset,
			const grfx::Buffer* pBuffer);

		virtual void PushComputeSampledImage(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			const grfx::SampledImageView* pView);

		virtual void PushComputeStorageImage(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			const grfx::StorageImageView* pView);

		virtual void PushComputeSampler(
			const grfx::PipelineInterface* pInterface,
			uint32_t                       binding,
			uint32_t                       set,
			const grfx::Sampler* pSampler);

		virtual void BindComputePipeline(const grfx::ComputePipeline* pPipeline) = 0;

		virtual void BindIndexBuffer(const grfx::IndexBufferView* pView) = 0;

		virtual void BindVertexBuffers(
			uint32_t                      viewCount,
			const grfx::VertexBufferView* pViews) = 0;

		virtual void Draw(
			uint32_t vertexCount,
			uint32_t instanceCount = 1,
			uint32_t firstVertex = 0,
			uint32_t firstInstance = 0) = 0;

		virtual void DrawIndexed(
			uint32_t indexCount,
			uint32_t instanceCount = 1,
			uint32_t firstIndex = 0,
			int32_t  vertexOffset = 0,
			uint32_t firstInstance = 0) = 0;

		virtual void Dispatch(
			uint32_t groupCountX,
			uint32_t groupCountY,
			uint32_t groupCountZ) = 0;

		virtual void CopyBufferToBuffer(
			const grfx::BufferToBufferCopyInfo* pCopyInfo,
			grfx::Buffer* pSrcBuffer,
			grfx::Buffer* pDstBuffer) = 0;

		virtual void CopyBufferToImage(
			const std::vector<grfx::BufferToImageCopyInfo>& pCopyInfos,
			grfx::Buffer* pSrcBuffer,
			grfx::Image* pDstImage) = 0;

		virtual void CopyBufferToImage(
			const grfx::BufferToImageCopyInfo* pCopyInfo,
			grfx::Buffer* pSrcBuffer,
			grfx::Image* pDstImage) = 0;

		//! @brief Copies an image to a buffer.
		//! @param pCopyInfo The specifications of the image region to copy.
		//! @param pSrcImage The source image.
		//! @param pDstBuffer The destination buffer.
		//! @return The image row pitch as written to the destination buffer.
		virtual grfx::ImageToBufferOutputPitch CopyImageToBuffer(
			const grfx::ImageToBufferCopyInfo* pCopyInfo,
			grfx::Image* pSrcImage,
			grfx::Buffer* pDstBuffer) = 0;

		virtual void CopyImageToImage(
			const grfx::ImageToImageCopyInfo* pCopyInfo,
			grfx::Image* pSrcImage,
			grfx::Image* pDstImage) = 0;

		virtual void BlitImage(
			const grfx::ImageBlitInfo* pCopyInfo,
			grfx::Image* pSrcImage,
			grfx::Image* pDstImage) = 0;

		virtual void BeginQuery(
			const grfx::Query* pQuery,
			uint32_t           queryIndex) = 0;

		virtual void EndQuery(
			const grfx::Query* pQuery,
			uint32_t           queryIndex) = 0;

		virtual void WriteTimestamp(
			const grfx::Query* pQuery,
			grfx::PipelineStage pipelineStage,
			uint32_t            queryIndex) = 0;

		virtual void ResolveQueryData(
			grfx::Query* pQuery,
			uint32_t     startIndex,
			uint32_t     numQueries) = 0;

		// ---------------------------------------------------------------------------------------------
		// Convenience functions
		// ---------------------------------------------------------------------------------------------
		void BeginRenderPass(const grfx::RenderPass* pRenderPass);

		void BeginRenderPass(
			const grfx::DrawPass* pDrawPass,
			const grfx::DrawPassClearFlags& clearFlags = grfx::DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

		virtual void TransitionImageLayout(
			const grfx::Texture* pTexture,
			uint32_t             mipLevel,
			uint32_t             mipLevelCount,
			uint32_t             arrayLayer,
			uint32_t             arrayLayerCount,
			grfx::ResourceState  beforeState,
			grfx::ResourceState  afterState,
			const grfx::Queue* pSrcQueue = nullptr,
			const grfx::Queue* pDstQueue = nullptr);

		void TransitionImageLayout(
			grfx::RenderPass* pRenderPass,
			grfx::ResourceState renderTargetBeforeState,
			grfx::ResourceState renderTargetAfterState,
			grfx::ResourceState depthStencilTargetBeforeState,
			grfx::ResourceState depthStencilTargetAfterState);

		void TransitionImageLayout(
			grfx::DrawPass* pDrawPass,
			grfx::ResourceState renderTargetBeforeState,
			grfx::ResourceState renderTargetAfterState,
			grfx::ResourceState depthStencilTargetBeforeState,
			grfx::ResourceState depthStencilTargetAfterState);

		void SetViewports(const grfx::Viewport& viewport);

		void SetScissors(const grfx::Rect& scissor);

		void BindIndexBuffer(const grfx::Buffer* pBuffer, grfx::IndexType indexType, uint64_t offset = 0);

		void BindIndexBuffer(const grfx::Mesh* pMesh, uint64_t offset = 0);

		void BindVertexBuffers(
			uint32_t                   bufferCount,
			const grfx::Buffer* const* buffers,
			const uint32_t* pStrides,
			const uint64_t* pOffsets = nullptr);

		void BindVertexBuffers(const grfx::Mesh* pMesh, const uint64_t* pOffsets = nullptr);

		//
		// NOTE: If you're running into an issue where VS2019 is incorrectly
		//       resolving call to this function to the Draw(vertexCount, ...)
		//       function above - it might be that the last parameter isn't
		//       explicitly the double pointer type. Possibly due to some casting.
		//
		//       For example this might give VS2019 some grief:
		//          grfx::DescriptorSetPtr set;
		//          Draw(quad, 1, set);
		//
		//       Use this instead:
		//          grfx::DescriptorSetPtr set;
		//          Draw(quad, 1, &set);
		//
		void Draw(const grfx::FullscreenQuad* pQuad, uint32_t setCount, const grfx::DescriptorSet* const* ppSets);

	protected:
		struct DynamicRenderPassInfo
		{
			grfx::Rect                             mRenderArea = {};
			std::vector<grfx::RenderTargetViewPtr> mRenderTargetViews = {};
			grfx::DepthStencilViewPtr              mDepthStencilView = nullptr;
		};

		// Returns true when inside a render pass (dynamic or regular)
		bool HasActiveRenderPass() const;

		bool                  mDynamicRenderPassActive = false;
		DynamicRenderPassInfo mDynamicRenderPassInfo = {};

	private:
		virtual void BeginRenderPassImpl(const grfx::RenderPassBeginInfo* pBeginInfo) = 0;
		virtual void EndRenderPassImpl() = 0;

		virtual void BeginRenderingImpl(const grfx::RenderingInfo* pRenderingInfo) = 0;
		virtual void EndRenderingImpl() = 0;

		virtual void PushDescriptorImpl(
			grfx::CommandType              pipelineBindPoint,
			const grfx::PipelineInterface* pInterface,
			grfx::DescriptorType           descriptorType,
			uint32_t                       binding,
			uint32_t                       set,
			uint32_t                       bufferOffset,
			const grfx::Buffer* pBuffer,
			const grfx::SampledImageView* pSampledImageView,
			const grfx::StorageImageView* pStorageImageView,
			const grfx::Sampler* pSampler) = 0;

		const grfx::RenderPass* mCurrentRenderPass = nullptr;
	};

} // namespace grfx

#pragma endregion

#pragma region Queue

namespace grfx {

	struct SubmitInfo
	{
		uint32_t                          commandBufferCount = 0;
		const grfx::CommandBuffer* const* ppCommandBuffers = nullptr;
		uint32_t                          waitSemaphoreCount = 0;
		const grfx::Semaphore* const* ppWaitSemaphores = nullptr;
		std::vector<uint64_t>             waitValues = {}; // Use 0 if index is binary semaphore
		uint32_t                          signalSemaphoreCount = 0;
		grfx::Semaphore** ppSignalSemaphores = nullptr;
		std::vector<uint64_t>             signalValues = {}; // Use 0 if index is binary smeaphore
		grfx::Fence* pFence = nullptr;
	};

	namespace internal {

		//! @struct QueueCreateInfo
		//!
		//!
		struct QueueCreateInfo
		{
			grfx::CommandType commandType = grfx::COMMAND_TYPE_UNDEFINED;
			uint32_t          queueFamilyIndex = VALUE_IGNORED; // Vulkan
			uint32_t          queueIndex = VALUE_IGNORED; // Vulkan
			void* pApiObject = nullptr;           // D3D12
		};

	} // namespace internal

	//! @class Queue
	//!
	//!
	class Queue
		: public grfx::DeviceObject<grfx::internal::QueueCreateInfo>
	{
	public:
		Queue() {}
		virtual ~Queue() {}

		grfx::CommandType GetCommandType() const { return mCreateInfo.commandType; }

		virtual Result WaitIdle() = 0;

		virtual Result Submit(const grfx::SubmitInfo* pSubmitInfo) = 0;

		// Timeline semaphore functions
		virtual Result QueueWait(grfx::Semaphore* pSemaphore, uint64_t value) = 0;
		virtual Result QueueSignal(grfx::Semaphore* pSemaphore, uint64_t value) = 0;

		// GPU timestamp frequency counter in ticks per second
		virtual Result GetTimestampFrequency(uint64_t* pFrequency) const = 0;

		Result CreateCommandBuffer(
			grfx::CommandBuffer** ppCommandBuffer,
			uint32_t              resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT,
			uint32_t              samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT);
		void DestroyCommandBuffer(const grfx::CommandBuffer* pCommandBuffer);

		// In place copy of buffer to buffer
		Result CopyBufferToBuffer(
			const grfx::BufferToBufferCopyInfo* pCopyInfo,
			grfx::Buffer* pSrcBuffer,
			grfx::Buffer* pDstBuffer,
			grfx::ResourceState                 stateBefore,
			grfx::ResourceState                 stateAfter);

		// In place copy of buffer to image
		Result CopyBufferToImage(
			const std::vector<grfx::BufferToImageCopyInfo>& pCopyInfos,
			grfx::Buffer* pSrcBuffer,
			grfx::Image* pDstImage,
			uint32_t                                        mipLevel,
			uint32_t                                        mipLevelCount,
			uint32_t                                        arrayLayer,
			uint32_t                                        arrayLayerCount,
			grfx::ResourceState                             stateBefore,
			grfx::ResourceState                             stateAfter);

	private:
		struct CommandSet
		{
			grfx::CommandPoolPtr   commandPool;
			grfx::CommandBufferPtr commandBuffer;
		};
		std::vector<CommandSet> mCommandSets;
		std::mutex              mCommandSetMutex;
	};

} // namespace grfx

#pragma endregion

#pragma region Scope

namespace grfx {

	class ScopeDestroyer
	{
	public:
		ScopeDestroyer(grfx::Device* pDevice);
		~ScopeDestroyer();

		Result AddObject(grfx::Image* pObject);
		Result AddObject(grfx::Buffer* pObject);
		Result AddObject(grfx::Mesh* pObject);
		Result AddObject(grfx::Texture* pObject);
		Result AddObject(grfx::Sampler* pObject);
		Result AddObject(grfx::SampledImageView* pObject);
		Result AddObject(grfx::Queue* pParent, grfx::CommandBuffer* pObject);

		// Releases all objects without destroying them
		void ReleaseAll();

	private:
		grfx::DevicePtr                                                mDevice;
		std::vector<grfx::ImagePtr>                                    mImages;
		std::vector<grfx::BufferPtr>                                   mBuffers;
		std::vector<grfx::MeshPtr>                                     mMeshes;
		std::vector<grfx::TexturePtr>                                  mTextures;
		std::vector<grfx::SamplerPtr>                                  mSamplers;
		std::vector<grfx::SampledImageViewPtr>                         mSampledImageViews;
		std::vector<std::pair<grfx::QueuePtr, grfx::CommandBufferPtr>> mTransientCommandBuffers;
	};

} // namespace grfx

#pragma endregion

#pragma region FullscreenQuad


/*

//
// Shaders for use with FullscreenQuad helper class should look something
// like the example shader below.
//
// Reference:
//   https://www.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau
//

Texture2D    Tex0     : register(t0);
SamplerState Sampler0 : register(s1);

struct VSOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(uint id : SV_VertexID)
{
	VSOutput result;

	// Clip space position
	result.Position.x = (float)(id / 2) * 4.0 - 1.0;
	result.Position.y = (float)(id % 2) * 4.0 - 1.0;
	result.Position.z = 0.0;
	result.Position.w = 1.0;

	// Texture coordinates
	result.TexCoord.x = (float)(id / 2) * 2.0;
	result.TexCoord.y = 1.0 - (float)(id % 2) * 2.0;

	return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
	return Tex0.Sample(Sampler0, input.TexCoord);
}

*/

namespace grfx {

	//! @struct FullscreenQuadCreateInfo
	//!
	//!
	struct FullscreenQuadCreateInfo
	{
		grfx::ShaderModule* VS = nullptr;
		grfx::ShaderModule* PS = nullptr;

		uint32_t setCount = 0;
		struct
		{
			uint32_t                   set = VALUE_IGNORED;
			grfx::DescriptorSetLayout* pLayout;
		} sets[MAX_BOUND_DESCRIPTOR_SETS] = {};

		uint32_t     renderTargetCount = 0;
		grfx::Format renderTargetFormats[MAX_RENDER_TARGETS] = { grfx::FORMAT_UNDEFINED };
		grfx::Format depthStencilFormat = grfx::FORMAT_UNDEFINED;
	};

	//! @class FullscreenQuad
	//!
	//!
	class FullscreenQuad
		: public grfx::DeviceObject<grfx::FullscreenQuadCreateInfo>
	{
	public:
		FullscreenQuad() {}
		virtual ~FullscreenQuad() {}

		grfx::PipelineInterfacePtr GetPipelineInterface() const { return mPipelineInterface; }
		grfx::GraphicsPipelinePtr  GetPipeline() const { return mPipeline; }

	protected:
		virtual Result CreateApiObjects(const grfx::FullscreenQuadCreateInfo* pCreateInfo) override;
		virtual void   DestroyApiObjects() override;

	private:
		grfx::PipelineInterfacePtr mPipelineInterface;
		grfx::GraphicsPipelinePtr  mPipeline;
	};

} // namespace grfx

#pragma endregion

#pragma region Mesh

namespace grfx {

	struct MeshVertexAttribute
	{
		grfx::Format format = grfx::FORMAT_UNDEFINED;

		// Use 0 to have stride calculated from format
		uint32_t stride = 0;

		// Not used for mesh/vertex buffer creation.
		// Gets calculated during creation for queries afterwards.
		uint32_t offset = 0;

		// [OPTIONAL] Useful for debugging.
		grfx::VertexSemantic vertexSemantic = grfx::VERTEX_SEMANTIC_UNDEFINED;
	};

	struct MeshVertexBufferDescription
	{
		uint32_t                  attributeCount = 0;
		grfx::MeshVertexAttribute attributes[MAX_VERTEX_BINDINGS] = {};

		// Use 0 to have stride calculated from attributes
		uint32_t stride = 0;

		grfx::VertexInputRate vertexInputRate = grfx::VERTEX_INPUT_RATE_VERTEX;
	};

	//! @struct MeshCreateInfo
	//!
	//! Usage Notes:
	//!   - Index and vertex data configuration needs to make sense
	//!       - If \b indexCount is 0 then \b vertexCount cannot be 0
	//!   - To create a mesh without an index buffer, \b indexType must be grfx::INDEX_TYPE_UNDEFINED
	//!   - If \b vertexCount is 0 then no vertex buffers will be created
	//!       - This means vertex buffer information will be ignored
	//!   - Active elements in \b vertexBuffers cannot have an \b attributeCount of 0
	//!
	struct MeshCreateInfo
	{
		grfx::IndexType                   indexType = grfx::INDEX_TYPE_UNDEFINED;
		uint32_t                          indexCount = 0;
		uint32_t                          vertexCount = 0;
		uint32_t                          vertexBufferCount = 0;
		grfx::MeshVertexBufferDescription vertexBuffers[MAX_VERTEX_BINDINGS] = {};
		grfx::MemoryUsage                 memoryUsage = grfx::MEMORY_USAGE_GPU_ONLY;

		MeshCreateInfo() {}
		MeshCreateInfo(const Geometry& geometry);
	};

	//! @class Mesh
	//!
	//! The \b Mesh class is a straight forward geometry container for the GPU.
	//! A \b Mesh instance consists of vertex data and an optional index buffer.
	//! The vertex data is stored in on or more vertex buffers. Each vertex buffer
	//! can store data for one or more attributes. The index data is stored in an
	//! index buffer.
	//!
	//! A \b Mesh instance does not store vertex binding information. Even if the
	//! create info is derived from a Geometry instance. This design is
	//! intentional since it enables calling applications to map vertex attributes
	//! and vertex buffers to how it sees fit. For convenience, the function
	//! \b Mesh::GetDerivedVertexBindings() returns vertex bindings derived from
	//! a \Mesh instance's vertex buffer descriptions.
	//!
	class Mesh
		: public grfx::DeviceObject<grfx::MeshCreateInfo>
	{
	public:
		Mesh() {}
		virtual ~Mesh() {}

		grfx::IndexType GetIndexType() const { return mCreateInfo.indexType; }
		uint32_t        GetIndexCount() const { return mCreateInfo.indexCount; }
		grfx::BufferPtr GetIndexBuffer() const { return mIndexBuffer; }

		uint32_t                                 GetVertexCount() const { return mCreateInfo.vertexCount; }
		uint32_t                                 GetVertexBufferCount() const { return CountU32(mVertexBuffers); }
		grfx::BufferPtr                          GetVertexBuffer(uint32_t index) const;
		const grfx::MeshVertexBufferDescription* GetVertexBufferDescription(uint32_t index) const;

		//! Returns derived vertex bindings based on the vertex buffer description
		const std::vector<grfx::VertexBinding>& GetDerivedVertexBindings() const { return mDerivedVertexBindings; }

	protected:
		virtual Result CreateApiObjects(const grfx::MeshCreateInfo* pCreateInfo) override;
		virtual void   DestroyApiObjects() override;

	private:
		grfx::BufferPtr                                                            mIndexBuffer;
		std::vector<std::pair<grfx::BufferPtr, grfx::MeshVertexBufferDescription>> mVertexBuffers;
		std::vector<grfx::VertexBinding>                                           mDerivedVertexBindings;
	};

} // namespace grfx

#pragma endregion

#pragma region Text Draw

namespace grfx {

	struct TextureFontUVRect
	{
		float u0 = 0;
		float v0 = 0;
		float u1 = 0;
		float v1 = 0;
	};

	struct TextureFontGlyphMetrics
	{
		uint32_t          codepoint = 0;
		GlyphMetrics      glyphMetrics = {};
		float2            size = {};
		TextureFontUVRect uvRect = {};
	};

	struct TextureFontCreateInfo
	{
		Font   font;
		float       size = 16.0f;
		std::string characters = ""; // Default characters if empty
	};

	class TextureFont
		: public grfx::DeviceObject<grfx::TextureFontCreateInfo>
	{
	public:
		TextureFont() {}
		virtual ~TextureFont() {}

		static std::string GetDefaultCharacters();

		const Font& GetFont() const { return mCreateInfo.font; }
		float            GetSize() const { return mCreateInfo.size; }
		std::string      GetCharacters() const { return mCreateInfo.characters; }
		grfx::TexturePtr GetTexture() const { return mTexture; }

		float                                GetAscent() const { return mFontMetrics.ascent; }
		float                                GetDescent() const { return mFontMetrics.descent; }
		float                                GetLineGap() const { return mFontMetrics.lineGap; }
		const grfx::TextureFontGlyphMetrics* GetGlyphMetrics(uint32_t codepoint) const;

	protected:
		virtual Result CreateApiObjects(const grfx::TextureFontCreateInfo* pCreateInfo) override;
		virtual void   DestroyApiObjects() override;

	private:
		FontMetrics                                mFontMetrics;
		std::vector<grfx::TextureFontGlyphMetrics> mGlyphMetrics;
		grfx::TexturePtr                           mTexture;
	};

	// -------------------------------------------------------------------------------------------------

	struct TextDrawCreateInfo
	{
		grfx::TextureFont* pFont = nullptr;
		uint32_t              maxTextLength = 4096;
		grfx::ShaderStageInfo VS = {}; // Use basic/shaders/TextDraw.hlsl (vsmain) for now
		grfx::ShaderStageInfo PS = {}; // Use basic/shaders/TextDraw.hlsl (psmain) for now
		grfx::BlendMode       blendMode = grfx::BLEND_MODE_PREMULT_ALPHA;
		grfx::Format          renderTargetFormat = grfx::FORMAT_UNDEFINED;
		grfx::Format          depthStencilFormat = grfx::FORMAT_UNDEFINED;
	};

	class TextDraw
		: public grfx::DeviceObject<grfx::TextDrawCreateInfo>
	{
	public:
		TextDraw() {}
		virtual ~TextDraw() {}

		void Clear();

		void AddString(
			const float2& position,
			const std::string& string,
			float              tabSpacing,  // Tab size, 0.5f = 0.5x space, 1.0 = 1x space, 2.0 = 2x space, etc
			float              lineSpacing, // Line spacing (ascent - descent + line gap), 0.5f = 0.5x line space, 1.0 = 1x line space, 2.0 = 2x line space, etc
			const float3& color,
			float              opacity);

		void AddString(
			const float2& position,
			const std::string& string,
			const float3& color = float3(1, 1, 1),
			float              opacity = 1.0f);

		// Use this if text is static
		Result UploadToGpu(grfx::Queue* pQueue);

		// Use this if text is dynamic
		void UploadToGpu(grfx::CommandBuffer* pCommandBuffer);

		void PrepareDraw(const float4x4& MVP, grfx::CommandBuffer* pCommandBuffer);
		void Draw(grfx::CommandBuffer* pCommandBuffer);

	protected:
		virtual Result CreateApiObjects(const grfx::TextDrawCreateInfo* pCreateInfo) override;
		virtual void   DestroyApiObjects() override;

	private:
		uint32_t                     mTextLength = 0;
		grfx::BufferPtr              mCpuIndexBuffer;
		grfx::BufferPtr              mCpuVertexBuffer;
		grfx::BufferPtr              mGpuIndexBuffer;
		grfx::BufferPtr              mGpuVertexBuffer;
		grfx::IndexBufferView        mIndexBufferView = {};
		grfx::VertexBufferView       mVertexBufferView = {};
		grfx::BufferPtr              mCpuConstantBuffer;
		grfx::BufferPtr              mGpuConstantBuffer;
		grfx::DescriptorPoolPtr      mDescriptorPool;
		grfx::DescriptorSetLayoutPtr mDescriptorSetLayout;
		grfx::DescriptorSetPtr       mDescriptorSet;
		grfx::PipelineInterfacePtr   mPipelineInterface;
		grfx::GraphicsPipelinePtr    mPipeline;
	};

} // namespace grfx

#pragma endregion

#pragma region MipMap


//! @class MipMap
//!
//! Stores a mipmap as a linear chunk of memory with each mip level accessible
//! as a Bitmap.
//!
//! The expected disk format used by Mipmap::LoadFile is an vertically tailed
//! mip map:
//!   +---------------------+
//!   | MIP 0               |
//!   |                     |
//!   +---------------------+
//!   | MIP 1    |          |
//!   +----------+----------+
//!   | ... |               |
//!   +-----+---------------+
//!
class Mipmap
{
public:
	Mipmap() {}
	// Using the static, shared-memory pool is currently only safe in single-threaded applications!
	// This should only be used for temporary mipmaps which will be destroyed prior to the creation of any new mipmap.
	Mipmap(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount, bool useStaticPool);
	Mipmap(uint32_t width, uint32_t height, Bitmap::Format format, uint32_t levelCount);
	// Using the static, shared-memory pool is currently only safe in single-threaded applications!
	// This should only be used for temporary mipmaps which will be destroyed prior to the creation of any new mipmap.
	Mipmap(const Bitmap& bitmap, uint32_t levelCount, bool useStaticPool);
	Mipmap(const Bitmap& bitmap, uint32_t levelCount);
	~Mipmap() {}

	// Returns true if there's at least one mip level, format is valid, and storage is valid
	bool IsOk() const;

	Bitmap::Format GetFormat() const;
	uint32_t       GetLevelCount() const { return CountU32(mMips); }
	Bitmap* GetMip(uint32_t level);
	const Bitmap* GetMip(uint32_t level) const;

	uint32_t GetWidth(uint32_t level) const;
	uint32_t GetHeight(uint32_t level) const;

	static uint32_t CalculateLevelCount(uint32_t width, uint32_t height);
	static Result   LoadFile(const std::filesystem::path& path, uint32_t baseWidth, uint32_t baseHeight, Mipmap* pMipmap, uint32_t levelCount = REMAINING_MIP_LEVELS);
	static Result   SaveFile(const std::filesystem::path& path, const Mipmap* pMipmap, uint32_t levelCount = REMAINING_MIP_LEVELS);

private:
	std::vector<char>   mData;
	std::vector<Bitmap> mMips;

	// Static, shared-memory pool for temporary mipmap generation.
	// NOTE: This is designed for single-threaded use ONLY as it's an unprotected memory block!
	// This will need locks if the consuming paths ever become multi-threaded.
	static std::vector<char> mStaticData;
	bool                     mUseStaticPool = false;
};

#pragma endregion

#pragma region generate_mip_shader_VK


#if 0
RWTexture2D<float4> dstTex : register(u0); // Destination texture

cbuffer ShaderConstantData : register(b1) {

	float2 texel_size;	// 1.0 / srcTex.Dimensions
	int src_mip_level;
	// Case to filter according the parity of the dimensions in the src texture. 
	// Must be one of 0, 1, 2 or 3
	// See CSMain function bellow
	int dimension_case;
	// Ignored for now, if we want to use a different filter strategy. Current one is bi-linear filter
	int filter_option;
};

Texture2D<float4> srcTex : register(t2); // Source texture

SamplerState LinearClampSampler : register(s3); // Sampler state

// According to the dimensions of the src texture we can be in one of four cases
float3 computePixelEvenEven(float2 scrCoords);
float3 computePixelEvenOdd(float2 srcCoords);
float3 computePixelOddEven(float2 srcCoords);
float3 computePixelOddOdd(float2 srcCoords);

[numthreads(1, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
	// Calculate the coordinates of the top left corner of the neighbourhood
	float2 coordInSrc = ((2 * dispatchThreadID.xy) * texel_size) + 0.5 * texel_size;

	float3 resultingPixel = float3(0.0f, 0.0f, 0.0f);
	// Get the filtered value from the src texture's neighbourhood
	// Choose the correct case according to src texture dimensions
	switch (dimension_case) {
	case 0: // Both dimension are even
		resultingPixel = computePixelEvenEven(coordInSrc);
		break;
	case 1: // width is even and height is odd
		resultingPixel = computePixelEvenOdd(coordInSrc);
		break;
	case 2: // width is odd an height is even
		resultingPixel = computePixelOddEven(coordInSrc);
		break;
	case 3: // both dimensions are odd
		resultingPixel = computePixelOddOdd(coordInSrc);
		break;
	}
	// Write the resulting color into dst texture
	dstTex[dispatchThreadID.xy] = float4(resultingPixel, 1.0f);
}


// In this case both dimensions (width and height) are even
// srcCoor are the coordinates of the top left corner of the neighbourhood in the src texture
float3 computePixelEvenEven(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 2x2 neighbourhood sampling
	const float2 neighboursCoords[2][2] = {
		{ {0.0, 0.0},          {texel_size.x, 0.0} },
		{ {0.0, texel_size.y}, {texel_size.x, texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average 1/4 = 0.25 
	const float coeficients[2][2] = {
									  { 0.25f, 0.25f },
									  { 0.25f, 0.25f }
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 2; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}

// In this case width is even and height is odd
// srcCoor are the coordinates of the top left corner of the neighbourhood in the src texture
// This neighbourhood has size 2x3 (in math matices notation)
float3 computePixelEvenOdd(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 2x3 neighbourhood sampling
	const float2 neighboursCoords[2][3] = {
		{ {0.0,          0.0}, {texel_size.x,          0.0}, {2.0 * texel_size.x, 0.0} },
		{ {0.0, texel_size.y}, {texel_size.x, texel_size.y}, {2.0 * texel_size.x, texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average. 1/4 = 0.25, 1/8 = 0.125
	const float coeficients[2][3] = {
									  { 0.125f, 0.25f, 0.125f},
									  { 0.125f, 0.25f, 0.125f}
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 3; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}

// In this case width is odd and height is even
// srcCoor are the coordinates of the top left corner of the neighbourhood in the src texture
// This neighbourhood has size 3x2 (in math matices notation)
float3 computePixelOddEven(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 3x2 neighbourhood sampling
	const float2 neighboursCoords[3][2] = {
		{ {0.0,                0.0}, {texel_size.x,               0.0f} },
		{ {0.0,       texel_size.y}, {texel_size.x,       texel_size.y} },
		{ {0.0, 2.0 * texel_size.y}, {texel_size.x, 2.0 * texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average. 1/4 = 0.25, 1/8 = 0.125
	const float coeficients[3][2] = {
									  { 0.125f, 0.125f },
									  { 0.25f,  0.25f },
									  { 0.125f, 0.125f }
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 2; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}

// In this case both width and height are odd
// srcCoor are the coordinates of the higher left corner of the neighbourhood in the src texture
// This neighbourhood has size 3x3 (in math matices notation)
float3 computePixelOddOdd(float2 srcCoords) {
	float3 resultPixel = float3(0.0f, 0.0f, 0.0f);
	//We will need a 3x3 neighbourhood sampling	
	const float2 neighboursCoords[3][3] = {
		{ {0.0,                0.0}, {texel_size.x,                0.0}, {2.0 * texel_size.x,                0.0} },
		{ {0.0,       texel_size.y}, {texel_size.x,       texel_size.y}, {2.0 * texel_size.x,       texel_size.y} },
		{ {0.0, 2.0 * texel_size.y}, {texel_size.x, 2.0 * texel_size.y}, {2.0 * texel_size.x, 2.0 * texel_size.y} }
	};
	// Filter or kernell: These are the coeficients for the weighted average. 1/4 = 0.25, 1/8 = 0.125, 1/16 = 0.0625
	const float coeficients[3][3] = {
									  { 0.0625f, 0.125f, 0.0625f},
									  { 0.125f,  0.25f,  0.125f},
									  { 0.0625f,  0.125f, 0.0625f}
	};
	// Perform the filtering by convolution
	for (int j = 0; j < 3; j++) {
		for (int i = 0; i < 3; i++) {
			float2 sampleCoords = srcCoords + neighboursCoords[j][i];
			resultPixel += coeficients[j][i] * srcTex.SampleLevel(LinearClampSampler, sampleCoords, src_mip_level).xyz;
		}
	}
	return resultPixel;
}
#endif

// Generated with:
// dxc -spirv -fspv-reflect -T cs_6_0 -E CSMain -Fh generate_mip_shader_VK.h generate_mip_shader.hlsl
const unsigned char GenerateMipShaderVK[] = {
	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x1b, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x43, 0x53, 0x4d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x58, 0x02, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x32, 0x64, 0x2e, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x64, 0x73, 0x74, 0x54, 0x65, 0x78, 0x00, 0x00, 0x05, 0x00, 0x08, 0x00, 0x05, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x43, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x44, 0x61, 0x74, 0x61, 0x00, 0x06, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x65, 0x6c, 0x5f, 0x73, 0x69, 0x7a, 0x65, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x73, 0x72, 0x63, 0x5f, 0x6d, 0x69, 0x70, 0x5f, 0x6c, 0x65, 0x76, 0x65, 0x6c, 0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x64, 0x69, 0x6d, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x5f, 0x63, 0x61, 0x73, 0x65, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x66, 0x69, 0x6c, 0x74, 0x65, 0x72, 0x5f, 0x6f, 0x70, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x06, 0x00, 0x00, 0x00, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x43, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x44, 0x61, 0x74, 0x61, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x32, 0x64, 0x2e, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x73, 0x72, 0x63, 0x54, 0x65, 0x78, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x09, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x4c, 0x69, 0x6e, 0x65, 0x61, 0x72, 0x43, 0x6c, 0x61, 0x6d, 0x70, 0x53, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x72, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x43, 0x53, 0x4d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x74, 0x79, 0x70, 0x65, 0x2e, 0x73, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x64, 0x2e, 0x69, 0x6d, 0x61, 0x67, 0x65, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x12, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x15, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x15, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3e, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x2b, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3d, 0x19, 0x00, 0x09, 0x00, 0x03, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x21, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x19, 0x00, 0x09, 0x00, 0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x02, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x24, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x26, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x27, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x28, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x29, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x30, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x31, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x32, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x14, 0x00, 0x02, 0x00, 0x33, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x03, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x34, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x36, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x37, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x38, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x39, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x3d, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x40, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x41, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x04, 0x00, 0x42, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x43, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x21, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x22, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x30, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x39, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x3a, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x05, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x39, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x06, 0x00, 0x42, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x47, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x26, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x4d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x41, 0x00, 0x00, 0x00, 0x4e, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x43, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x3d, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x38, 0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x3b, 0x00, 0x00, 0x00, 0x53, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x31, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x24, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x07, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x84, 0x00, 0x05, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x70, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x29, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5d, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x5d, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x5f, 0x00, 0x00, 0x00, 0xf7, 0x00, 0x03, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x0b, 0x00, 0x60, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x62, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x6a, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x6a, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x6d, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2d, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x00, 0x6e, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x54, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x55, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x70, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x70, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0x77, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0x76, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x78, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x79, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x79, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x85, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0x8b, 0x00, 0x00, 0x00, 0x55, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x00, 0x8b, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x8d, 0x00, 0x00, 0x00, 0x8a, 0x00, 0x00, 0x00, 0x8c, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x7a, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00, 0x00, 0x8d, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x79, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x7f, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x73, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x73, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x70, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x77, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x63, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x96, 0x00, 0x00, 0x00, 0x97, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x37, 0x00, 0x00, 0x00, 0x9a, 0x00, 0x00, 0x00, 0x95, 0x00, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x52, 0x00, 0x00, 0x00, 0x9a, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x53, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x9b, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x9b, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xa2, 0x00, 0x00, 0x00, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xa1, 0x00, 0x00, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xa2, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xa4, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa4, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xa5, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xa3, 0x00, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xa9, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xaa, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xa9, 0x00, 0x00, 0x00, 0xa6, 0x00, 0x00, 0x00, 0xaa, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa6, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0xab, 0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0xac, 0x00, 0x00, 0x00, 0xab, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xad, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0xac, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0xae, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0xaf, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xb2, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xb3, 0x00, 0x00, 0x00, 0xae, 0x00, 0x00, 0x00, 0xaf, 0x00, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0xb3, 0x00, 0x00, 0x00, 0xad, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xb2, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0xb5, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0xb6, 0x00, 0x00, 0x00, 0x53, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xb7, 0x00, 0x00, 0x00, 0xb6, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0xb5, 0x00, 0x00, 0x00, 0xb7, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xa5, 0x00, 0x00, 0x00, 0x9d, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xa8, 0x00, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xa4, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xaa, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x9e, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x9e, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x00, 0x9f, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x9b, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xa2, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xb9, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0xb9, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0xbb, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0xbd, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0xbe, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc1, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xc1, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xbd, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0xba, 0x00, 0x00, 0x00, 0xbd, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x2c, 0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x3c, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0xc2, 0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x50, 0x00, 0x00, 0x00, 0xc6, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x51, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xc7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xc7, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0xc9, 0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xcd, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xce, 0x00, 0x00, 0x00, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xcd, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xce, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xc9, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xcf, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xd5, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xd5, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0xd7, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0xd7, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xd9, 0x00, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0xd8, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0xda, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0xdb, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0xdd, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xde, 0x00, 0x00, 0x00, 0xdd, 0x00, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xdf, 0x00, 0x00, 0x00, 0xda, 0x00, 0x00, 0x00, 0xdb, 0x00, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xdf, 0x00, 0x00, 0x00, 0xd9, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xde, 0x00, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0xe1, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0xe2, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe3, 0x00, 0x00, 0x00, 0xe2, 0x00, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 0x00, 0xe1, 0x00, 0x00, 0x00, 0xe3, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0xd1, 0x00, 0x00, 0x00, 0xc9, 0x00, 0x00, 0x00, 0xe4, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xd4, 0x00, 0x00, 0x00, 0xd3, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xd6, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xca, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xca, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xcc, 0x00, 0x00, 0x00, 0xcb, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xc7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xce, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x65, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xe5, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0xe5, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x32, 0x00, 0x00, 0x00, 0xe8, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0xe8, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xeb, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0xed, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0xeb, 0x00, 0x00, 0x00, 0xec, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xef, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0xf1, 0x00, 0x00, 0x00, 0xee, 0x00, 0x00, 0x00, 0xef, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf2, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf3, 0x00, 0x00, 0x00, 0xe6, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x50, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0xe7, 0x00, 0x00, 0x00, 0xea, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x36, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x00, 0x00, 0xf2, 0x00, 0x00, 0x00, 0xf3, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0x50, 0x00, 0x06, 0x00, 0x40, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00, 0x00, 0xed, 0x00, 0x00, 0x00, 0xf1, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x4e, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00, 0x4f, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xf7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xf7, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0xfd, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xff, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x15, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xf5, 0x00, 0x07, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0xb1, 0x00, 0x05, 0x00, 0x33, 0x00, 0x00, 0x00, 0x05, 0x01, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0xf6, 0x00, 0x04, 0x00, 0x06, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfa, 0x00, 0x04, 0x00, 0x05, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x06, 0x01, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x02, 0x01, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x28, 0x00, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x4e, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x20, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x07, 0x01, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x20, 0x00, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x5e, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0a, 0x01, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0b, 0x01, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x2a, 0x00, 0x00, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00, 0x0d, 0x01, 0x00, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x6f, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x0e, 0x01, 0x00, 0x00, 0x0d, 0x01, 0x00, 0x00, 0x56, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x0a, 0x01, 0x00, 0x00, 0x0b, 0x01, 0x00, 0x00, 0x58, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0e, 0x01, 0x00, 0x00, 0x4f, 0x00, 0x08, 0x00, 0x15, 0x00, 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x41, 0x00, 0x06, 0x00, 0x34, 0x00, 0x00, 0x00, 0x12, 0x01, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x12, 0x00, 0x00, 0x00, 0x13, 0x01, 0x00, 0x00, 0x12, 0x01, 0x00, 0x00, 0x8e, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x13, 0x01, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x15, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0xf9, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x03, 0x01, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x06, 0x01, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xfa, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xfa, 0x00, 0x00, 0x00, 0x80, 0x00, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0xfb, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0xf7, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0xfe, 0x00, 0x00, 0x00, 0xf9, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x61, 0x00, 0x00, 0x00, 0xf5, 0x00, 0x0d, 0x00, 0x15, 0x00, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x4d, 0x00, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x77, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0xa2, 0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0xce, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x16, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x17, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x12, 0x00, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x15, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x50, 0x00, 0x07, 0x00, 0x2b, 0x00, 0x00, 0x00, 0x19, 0x01, 0x00, 0x00, 0x16, 0x01, 0x00, 0x00, 0x17, 0x01, 0x00, 0x00, 0x18, 0x01, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1a, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x63, 0x00, 0x05, 0x00, 0x1a, 0x01, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x19, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00 };

#pragma endregion

#pragma region generate_mip_shader_DX

#pragma endregion


#pragma region Graphics Utils

namespace grfx_util {

	class ImageOptions
	{
	public:
		ImageOptions() {}
		~ImageOptions() {}

		// clang-format off
		ImageOptions& AdditionalUsage(grfx::ImageUsageFlags flags) { mAdditionalUsage = flags; return *this; }
		ImageOptions& MipLevelCount(uint32_t levelCount) { mMipLevelCount = levelCount; return *this; }
		// clang-format on

	private:
		grfx::ImageUsageFlags mAdditionalUsage = grfx::ImageUsageFlags();
		uint32_t              mMipLevelCount = REMAINING_MIP_LEVELS;

		friend Result CreateImageFromBitmap(
			grfx::Queue* pQueue,
			const Bitmap* pBitmap,
			grfx::Image** ppImage,
			const ImageOptions& options);

		friend Result CreateImageFromCompressedImage(
			grfx::Queue* pQueue,
			const gli::texture& image,
			grfx::Image** ppImage,
			const ImageOptions& options);

		friend Result CreateImageFromFile(
			grfx::Queue* pQueue,
			const std::filesystem::path& path,
			grfx::Image** ppImage,
			const ImageOptions& options,
			bool                         useGpu);

		friend Result CreateImageFromBitmapGpu(
			grfx::Queue* pQueue,
			const Bitmap* pBitmap,
			grfx::Image** ppImage,
			const ImageOptions& options);
	};

	//! @fn CopyBitmapToImage
	//!
	//!
	Result CopyBitmapToImage(
		grfx::Queue* pQueue,
		const Bitmap* pBitmap,
		grfx::Image* pImage,
		uint32_t            mipLevel,
		uint32_t            arrayLayer,
		grfx::ResourceState stateBefore,
		grfx::ResourceState stateAfter);

	//! @fn CreateImageFromBitmap
	//!
	//!
	Result CreateImageFromBitmap(
		grfx::Queue* pQueue,
		const Bitmap* pBitmap,
		grfx::Image** ppImage,
		const ImageOptions& options = ImageOptions());

	//! @fn CreateImageFromFile
	//!
	//!
	Result CreateImageFromFile(
		grfx::Queue* pQueue,
		const std::filesystem::path& path,
		grfx::Image** ppImage,
		const ImageOptions& options = ImageOptions(),
		bool                         useGpu = false);

	//! @fn CreateMipMapsForImage
	//!
	//!
	Result CreateImageFromBitmapGpu(
		grfx::Queue* pQueue,
		const Bitmap* pBitmap,
		grfx::Image** ppImage,
		const ImageOptions& options = ImageOptions());

	// -------------------------------------------------------------------------------------------------

	class TextureOptions
	{
	public:
		TextureOptions() {}
		~TextureOptions() {}

		// clang-format off
		TextureOptions& AdditionalUsage(grfx::ImageUsageFlags flags) { mAdditionalUsage = flags; return *this; }
		TextureOptions& InitialState(grfx::ResourceState state) { mInitialState = state; return *this; }
		TextureOptions& MipLevelCount(uint32_t levelCount) { mMipLevelCount = levelCount; return *this; }
		TextureOptions& SamplerYcbcrConversion(grfx::SamplerYcbcrConversion* pYcbcrConversion) { mYcbcrConversion = pYcbcrConversion; return *this; }
		// clang-format on

	private:
		grfx::ImageUsageFlags         mAdditionalUsage = grfx::ImageUsageFlags();
		grfx::ResourceState           mInitialState = grfx::ResourceState::RESOURCE_STATE_SHADER_RESOURCE;
		uint32_t                      mMipLevelCount = 1;
		grfx::SamplerYcbcrConversion* mYcbcrConversion = nullptr;

		friend Result CreateTextureFromBitmap(
			grfx::Queue* pQueue,
			const Bitmap* pBitmap,
			grfx::Texture** ppTexture,
			const TextureOptions& options);

		friend Result CreateTextureFromMipmap(
			grfx::Queue* pQueue,
			const Mipmap* pMipmap,
			grfx::Texture** ppTexture,
			const TextureOptions& options);

		friend Result CreateTextureFromFile(
			grfx::Queue* pQueue,
			const std::filesystem::path& path,
			grfx::Texture** ppTexture,
			const TextureOptions& options);
	};

	//! @fn CreateTextureFromBitmap
	//!
	//!
	Result CopyBitmapToTexture(
		grfx::Queue* pQueue,
		const Bitmap* pBitmap,
		grfx::Texture* pTexture,
		uint32_t            mipLevel,
		uint32_t            arrayLayer,
		grfx::ResourceState stateBefore,
		grfx::ResourceState stateAfter);

	//! @fn CreateTextureFromBitmap
	//!
	//!
	Result CreateTextureFromBitmap(
		grfx::Queue* pQueue,
		const Bitmap* pBitmap,
		grfx::Texture** ppTexture,
		const TextureOptions& options = TextureOptions());

	//! @fn CreateTextureFromMipmap
	//!
	//! Mip level count from pMipmap is used. Mip level count from options is ignored.
	//!
	Result CreateTextureFromMipmap(
		grfx::Queue* pQueue,
		const Mipmap* pMipmap,
		grfx::Texture** ppTexture,
		const TextureOptions& options = TextureOptions());

	//! @fn CreateTextureFromFile
	//!
	//!
	Result CreateTextureFromFile(
		grfx::Queue* pQueue,
		const std::filesystem::path& path,
		grfx::Texture** ppTexture,
		const TextureOptions& options = TextureOptions());

	// Create a 1x1 texture with the specified pixel data. The format
	// for the texture is derived from the pixel data type, which
	// can be one of uint8, uint16, uint32 or float.
	template <typename PixelDataType>
	Result CreateTexture1x1(
		grfx::Queue* pQueue,
		const std::array<PixelDataType, 4> color,
		grfx::Texture** ppTexture,
		const TextureOptions& options = TextureOptions())
	{
		ASSERT_NULL_ARG(pQueue);
		ASSERT_NULL_ARG(ppTexture);

		Bitmap::Format format = Bitmap::FORMAT_UNDEFINED;
		if constexpr (std::is_same_v<PixelDataType, float>) {
			format = Bitmap::FORMAT_RGBA_FLOAT;
		}
		else if constexpr (std::is_same_v<PixelDataType, uint32_t>) {
			format = Bitmap::FORMAT_RGBA_UINT32;
		}
		else if constexpr (std::is_same_v<PixelDataType, uint16_t>) {
			format = Bitmap::FORMAT_RGBA_UINT16;
		}
		else if constexpr (std::is_same_v<PixelDataType, uint8_t>) {
			format = Bitmap::FORMAT_RGBA_UINT8;
		}
		else {
			ASSERT_MSG(false, "Invalid pixel data type: must be one of uint8, uint16, uint32 or float.")
				return ERROR_INVALID_CREATE_ARGUMENT;
		}

		// Create bitmap
		Result ppxres = SUCCESS;
		Bitmap bitmap = Bitmap::Create(1, 1, format, &ppxres);
		if (Failed(ppxres)) {
			return ERROR_BITMAP_CREATE_FAILED;
		}

		// Fill color
		bitmap.Fill(color[0], color[1], color[2], color[3]);

		return CreateTextureFromBitmap(pQueue, &bitmap, ppTexture, options);
	}

	// Creates an irradiance and an environment texture from an *.ibl file
	Result CreateIBLTexturesFromFile(
		grfx::Queue* pQueue,
		const std::filesystem::path& path,
		grfx::Texture** ppIrradianceTexture,
		grfx::Texture** ppEnvironmentTexture);

	// -------------------------------------------------------------------------------------------------

	// clang-format off
	/*

	Cross Horizontal Left:
			   _____
			  |  0  |
		 _____|_____|_____ _____
		|  1  |  2  |  3  |  4  |
		|_____|_____|_____|_____|
			  |  5  |
			  |_____|

	Cross Horizontal Right:
					 _____
					|  0  |
		 ___________|_____|_____
		|  1  |  2  |  3  |  4  |
		|_____|_____|_____|_____|
					|  5  |
					|_____|

	Cross Vertical Top:
			   _____
			  |  0  |
		 _____|_____|_____
		|  1  |  2  |  3  |
		|_____|_____|_____|
			  |  4  |
			  |_____|
			  |  5  |
			  |_____|

	Cross Vertical Bottom:
			   _____
			  |  0  |
			  |_____|
			  |  1  |
		 _____|_____|_____
		|  2  |  3  |  4  |
		|_____|_____|_____|
			  |  5  |
			  |_____|

	Lat Long Horizontal:
		 _____ _____ _____
		|  0  |  1  |  2  |
		|_____|_____|_____|
		|  3  |  4  |  5  |
		|_____|_____|_____|

	Lat Long Vertical:
		 _____ _____
		|  0  |  1  |
		|_____|_____|
		|  2  |  3  |
		|_____|_____|
		|  4  |  5  |
		|_____|_____|

	Strip Horizontal:
		 _____ _____ _____ _____ _____ _____
		|  0  |  1  |  2  |  3  |  4  |  5  |
		|_____|_____|_____|_____|_____|_____|


	Strip Vertical:
		 _____
		|  0  |
		|_____|
		|  1  |
		|_____|
		|  2  |
		|_____|
		|  3  |
		|_____|
		|  4  |
		|_____|
		|  5  |
		|_____|

	*/
	// clang-format on
	enum CubeImageLayout
	{
		CUBE_IMAGE_LAYOUT_UNDEFINED = 0,
		CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_LEFT = 1,
		CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_RIGHT = 2,
		CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_TOP = 3,
		CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_BOTTOM = 4,
		CUBE_IMAGE_LAYOUT_LAT_LONG_HORIZONTAL = 5,
		CUBE_IMAGE_LAYOUT_LAT_LONG_VERTICAL = 6,
		CUBE_IMAGE_LAYOUT_STRIP_HORIZONTAL = 7,
		CUBE_IMAGE_LAYOUT_STRIP_VERTICAL = 8,
		CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL = CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL_LEFT,
		CUBE_IMAGE_LAYOUT_CROSS_VERTICAL = CUBE_IMAGE_LAYOUT_CROSS_VERTICAL_TOP,
		CUBE_IMAGE_LAYOUT_LAT_LONG = CUBE_IMAGE_LAYOUT_LAT_LONG_HORIZONTAL,
		CUBE_IMAGE_LAYOUT_STRIP = CUBE_IMAGE_LAYOUT_STRIP_HORIZONTAL,
	};

	//! @enum CubeImageLoadOp
	//!
	//! Rotation is always clockwise.
	//!
	enum CubeFaceOp
	{
		CUBE_FACE_OP_NONE = 0,
		CUBE_FACE_OP_ROTATE_90 = 1,
		CUBE_FACE_OP_ROTATE_180 = 2,
		CUBE_FACE_OP_ROTATE_270 = 3,
		CUBE_FACE_OP_INVERT_HORIZONTAL = 4,
		CUBE_FACE_OP_INVERT_VERTICAL = 5,
	};

	//! @struct CubeMapCreateInfo
	//!
	//! See enum CubeImageLayout for explanation of layouts
	//!
	//! Example - Use subimage 0 with 90 degrees CW rotation for posX face:
	//!   layout = CUBE_IMAGE_LAYOUT_CROSS_HORIZONTAL;
	//!   posX   = ENCODE_CUBE_FACE(0, CUBE_FACE_OP_ROTATE_90, :CUBE_FACE_OP_NONE);
	//!
#define CUBE_OP_MASK           0xFF
#define CUBE_OP_SUBIMAGE_SHIFT 0
#define CUBE_OP_OP1_SHIFT      8
#define CUBE_OP_OP2_SHIFT      16

#define ENCODE_CUBE_FACE(SUBIMAGE, OP1, OP2)              \
    (SUBIMAGE & CUBE_OP_MASK) |                           \
        ((OP1 & CUBE_OP_MASK) << CUBE_OP_OP1_SHIFT) | \
        ((OP1 & CUBE_OP_MASK) << CUBE_OP_OP2_SHIFT)

#define DECODE_CUBE_FACE_SUBIMAGE(FACE) (FACE >> CUBE_OP_SUBIMAGE_SHIFT) & CUBE_OP_MASK
#define DECODE_CUBE_FACE_OP1(FACE)      (FACE >> CUBE_OP_OP1_SHIFT) & CUBE_OP_MASK
#define DECODE_CUBE_FACE_OP2(FACE)      (FACE >> CUBE_OP_OP2_SHIFT) & CUBE_OP_MASK

//! @struct CubeMapCreateInfo
//!
//!
	struct CubeMapCreateInfo
	{
		CubeImageLayout layout = CUBE_IMAGE_LAYOUT_UNDEFINED;
		uint32_t        posX = VALUE_IGNORED;
		uint32_t        negX = VALUE_IGNORED;
		uint32_t        posY = VALUE_IGNORED;
		uint32_t        negY = VALUE_IGNORED;
		uint32_t        posZ = VALUE_IGNORED;
		uint32_t        negZ = VALUE_IGNORED;
	};

	//! @fn CreateCubeMapFromFile
	//!
	//!
	Result CreateCubeMapFromFile(
		grfx::Queue* pQueue,
		const std::filesystem::path& path,
		const CubeMapCreateInfo* pCreateInfo,
		grfx::Image** ppImage,
		const grfx::ImageUsageFlags& additionalImageUsage = grfx::ImageUsageFlags());

	// -------------------------------------------------------------------------------------------------

	//! @fn CreateMeshFromGeometry
	//!
	//!
	Result CreateMeshFromGeometry(
		grfx::Queue* pQueue,
		const Geometry* pGeometry,
		grfx::Mesh** ppMesh);

	//! @fn CreateMeshFromTriMesh
	//!
	//!
	Result CreateMeshFromTriMesh(
		grfx::Queue* pQueue,
		const TriMesh* pTriMesh,
		grfx::Mesh** ppMesh);

	//! @fn CreateMeshFromWireMesh
	//!
	//!
	Result CreateMeshFromWireMesh(
		grfx::Queue* pQueue,
		const WireMesh* pWireMesh,
		grfx::Mesh** ppMesh);

	//! @fn CreateModelFromFile
	//!
	//!
	Result CreateMeshFromFile(
		grfx::Queue* pQueue,
		const std::filesystem::path& path,
		grfx::Mesh** ppMesh,
		const TriMeshOptions& options = TriMeshOptions());

	// -------------------------------------------------------------------------------------------------

	grfx::Format ToGrfxFormat(Bitmap::Format value);

} // namespace grfx_util

#pragma endregion
