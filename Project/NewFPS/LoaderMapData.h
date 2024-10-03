#pragma once

class LoaderMapData;

namespace fileMapData
{
#define ENT_SPACING_DEFAULT 2.0f
#define TILE_SPACING_DEFAULT 2.0f
#define NO_TEX -1
#define NO_MODEL -1

	typedef int TexID;
	typedef int ModelID;
	enum class Direction { Z_POS, Z_NEG, X_POS, X_NEG, Y_POS, Y_NEG };

	struct Tile final
	{
		Tile() : shape(NO_MODEL), angle(0), texture(NO_TEX), pitch(0) {}
		Tile(ModelID s, int a, TexID t, int p) : shape(s), angle(a), texture(t), pitch(p) {}

		operator bool() const
		{
			return shape > NO_MODEL && texture > NO_TEX;
		}

		ModelID shape;
		int angle; //Yaw in whole number of degrees
		TexID texture;
		int pitch; //Pitch in whole number of degrees
	};

	inline bool operator==(const Tile& lhs, const Tile& rhs)
	{
		return (lhs.shape == rhs.shape) && (lhs.texture == rhs.texture) && (lhs.angle == rhs.angle) && (lhs.pitch == rhs.pitch);
	}

	inline bool operator!=(const Tile& lhs, const Tile& rhs)
	{
		return !(lhs == rhs);
	}

	//Represents a 3 dimensional array of tiles and provides functions for converting coordinates.
	template<class Cel>
	class Grid
	{
	public:
		//Constructs a blank grid of zero size.
		Grid() : Grid(0, 0, 0, 0.0f) {}
		//Constructs a grid filled with the given cel.
		Grid(size_t width, size_t height, size_t length, float spacing, const Cel& fill)
		{
			m_width = width; m_height = height; m_length = length; m_spacing = spacing;
			m_grid.resize(width * height * length);

			for (size_t i = 0; i < m_grid.size(); ++i) { m_grid[i] = fill; }
		}
		//Constructs a grid full of default-constructed cels.
		Grid(size_t width, size_t height, size_t length, float spacing) : Grid(width, height, length, spacing, Cel()) {}
		virtual ~Grid() = default;

		glm::vec3 WorldToGridPos(const glm::vec3& worldPos) const
		{
			return { 
				floorf(worldPos.x / m_spacing), 
				floorf(worldPos.y / m_spacing) , 
				floorf(worldPos.z / m_spacing)
			};
		}

		//Converts (whole number) grid cel coordinates to world coordinates.
		//If `center` is true, then the world coordinate will be in the center of the cel instead of the corner.
		glm::vec3 GridToWorldPos(const glm::vec3& gridPos, bool center) const
		{
			if (center)
			{
				return {
					(gridPos.x * m_spacing) + (m_spacing / 2.0f),
					(gridPos.y * m_spacing) + (m_spacing / 2.0f),
					(gridPos.z * m_spacing) + (m_spacing / 2.0f),
				};
			}
			else
			{
				return { gridPos.x * m_spacing, gridPos.y * m_spacing, gridPos.z * m_spacing };
			}
		}

		glm::vec3 SnapToCelCenter(const glm::vec3& worldPos) const
		{
			worldPos.x = (floorf(worldPos.x / m_spacing) * m_spacing) + (m_spacing / 2.0f);
			worldPos.y = (floorf(worldPos.y / m_spacing) * m_spacing) + (m_spacing / 2.0f);
			worldPos.z = (floorf(worldPos.z / m_spacing) * m_spacing) + (m_spacing / 2.0f);
			return worldPos;
		}

		size_t FlatIndex(int i, int j, int k) const
		{
			return i + (k * m_width) + (j * m_width * m_length);
		}

		glm::vec3 UnflattenIndex(size_t idx) const
		{
			return {
				(float)(idx % m_width),
				(float)(idx / (m_width * m_length)),
				(float)((idx / m_width) % m_length)
			};
		}

		size_t GetWidth() const { return m_width; }
		size_t GetHeight() const { return m_height; }
		size_t GetLength() const { return m_length; }
		float GetSpacing() const { return m_spacing; }

		glm::vec3 GetMinCorner() const
		{
			return { 0.0f };
		}

		glm::vec3 GetMaxCorner() const
		{
			return { (float)m_width * m_spacing, (float)m_height * m_spacing, (float)m_length * m_spacing };
		}

		glm::vec3 GetCenterPos() const
		{
			return { 
				(float)m_width * m_spacing / 2.0f, 
				(float)m_height * m_spacing / 2.0f, 
				(float)m_length * m_spacing / 2.0f
			};
		}

	protected:
		void setCel(int i, int j, int k, const Cel& cel)
		{
			m_grid[FlatIndex(i, j, k)] = cel;
		}

		Cel getCel(int i, int j, int k) const
		{
			return m_grid[FlatIndex(i, j, k)];
		}

		void copyCels(int i, int j, int k, const Grid<Cel>& src)
		{
			assert(i >= 0 && j >= 0 && k >= 0);
			int xEnd = glm::min(i + src.m_width, m_width);
			int yEnd = glm::min(j + src.m_height, m_height);
			int zEnd = glm::min(k + src.m_length, m_length);
			for (int z = k; z < zEnd; ++z)
			{
				for (int y = j; y < yEnd; ++y)
				{
					size_t ourBase = FlatIndex(0, y, z);
					size_t theirBase = src.FlatIndex(0, y - j, z - k);
					for (int x = i; x < xEnd; ++x)
					{
						const Cel& cel = src.m_grid[theirBase + (x - i)];
						m_grid[ourBase + x] = cel;
					}
				}
			}
		}

		void subsectionCopy(int i, int j, int k, int w, int h, int l, Grid<Cel>& out) const
		{
			for (int z = k; z < k + l; ++z)
			{
				for (int y = j; y < j + h; ++y)
				{
					size_t ourBase = FlatIndex(0, y, z);
					size_t theirBase = out.FlatIndex(0, y - j, z - k);
					for (int x = i; x < i + w; ++x)
					{
						out.m_grid[theirBase + (x - i)] = m_grid[ourBase + x];
					}
				}
			}
		}

		std::vector<Cel> m_grid;
		size_t m_width, m_height, m_length;
		float m_spacing;
	};

	class TileGrid : public Grid<Tile>
	{
	public:
		//Constructs a blank TileGrid with no size
		TileGrid() : TileGrid(nullptr, 0, 0, 0) {}
		//Constructs a TileGrid full of empty tiles.
		TileGrid(LoaderMapData* mapMan, size_t width, size_t height, size_t length) : TileGrid(mapMan, width, height, length, TILE_SPACING_DEFAULT, Tile{ NO_MODEL, 0, NO_TEX, 0 }) {}
		//Constructs a TileGrid filled with the given tile.
		TileGrid(LoaderMapData* mapMan, size_t width, size_t height, size_t length, float spacing, Tile fill);

		Tile GetTile(int i, int j, int k) const;
		Tile GetTile(int flatIndex) const;
		void SetTile(int i, int j, int k, const Tile& tile);
		void SetTile(int flatIndex, const Tile& tile);

		//Returns a base64 encoded string with the binary representations of all tiles.
		std::string GetTileDataBase64() const;

		std::string GetOptimizedTileDataBase64() const;

		//Assigns tiles based on the binary data encoded in base 64. Assumes that the sizes of the data and the current grid are the same.
		void SetTileDataBase64(std::string data);

		//Returns the list of texture and model IDs that are actually used in this tile grid
		std::pair<std::vector<TexID>, std::vector<ModelID>> GetUsedIDs() const;

	private:
		LoaderMapData* m_mapMan;

		glm::vec3 m_batchPosition;
		bool m_regenBatches;
		bool m_regenModel;
		int m_batchFromY;
		int m_batchToY;

		bool m_modelCulled;
	};

	//This is short for "entity", because "entity" is difficult to type.
	struct Ent
	{
		enum class DisplayMode {
			SPHERE,
			MODEL,
			SPRITE,
		};

		bool active;

		DisplayMode display;
		Color color;
		float radius;
		glm::vec3 position; //World space coordinates
		int yaw, pitch; //Degrees angle orientation

		//Entity properties are key/value pairs. No data types are enforced, values are parsed as strings.
		std::map<std::string, std::string> properties;

		Ent();
		Ent(float radius);
	};

	void to_json(nlohmann::json& j, const Ent& ent);
	void from_json(const nlohmann::json& j, Ent& ent);

	//This represents a grid of entities. 
	//Instead of the grid storing entities directly, it stores iterators into the `_ents` array to save on memory.
	class EntGrid : public Grid<Ent>
	{
	public:
		//Creates an empty entgrid of zero size
		EntGrid() : EntGrid(0, 0, 0) {}

		//Creates ent grid of given dimensions, default spacing.
		EntGrid(size_t width, size_t height, size_t length);

		//Will set the given ent to occupy the grid space, replacing any existing entity in that space.
		void AddEnt(int i, int j, int k, Ent ent)
		{
			ent.position = GridToWorldPos(glm::vec3{ (float)i, (float)j, (float)k }, true);
			setCel(i, j, k, ent);
		}

		void RemoveEnt(int i, int j, int k)
		{
			setCel(i, j, k, Ent());
		}

		bool HasEnt(int i, int j, int k) const
		{
			return getCel(i, j, k).active;
		}

		Ent GetEnt(int i, int j, int k) const
		{
			assert(HasEnt(i, j, k));
			return getCel(i, j, k);
		}

		void CopyEnts(int i, int j, int k, const EntGrid& src)
		{
			return copyCels(i, j, k, src);
		}

		//Returns a smaller grid with a copy of the ent data in the rectangle defined by coordinates (i, j, k) and size (w, h, l).
		EntGrid Subsection(int i, int j, int k, int w, int h, int l) const
		{
			assert(i >= 0 && j >= 0 && k >= 0);
			assert(i + w <= int(m_width) && j + h <= int(m_height) && k + l <= int(m_length));

			EntGrid newGrid(w, h, l);

			subsectionCopy(i, j, k, w, h, l, newGrid);

			return newGrid;
		}

		//Returns a contiguous array of all active entities.
		std::vector<Ent> GetEntList() const
		{
			std::vector<Ent> out;
			for (const Ent& ent : m_grid)
			{
				if (ent.active) out.push_back(ent);
			}
			return out;
		}
	private:
		std::vector<std::pair<glm::vec3, std::string>> m_labelsToDraw;
	};
}

class LoaderMapData final
{
public:
	bool Setup(std::filesystem::path filePath);
	void Shutdown();

private:
	std::vector<std::filesystem::path> m_texturePaths;
	std::vector<std::filesystem::path> m_modelPaths;

	fileMapData::TileGrid m_tileGrid;
	fileMapData::EntGrid m_entGrid;

	glm::vec3 m_defaultCameraPosition = glm::vec3(0.0f);
	glm::vec3 m_defaultCameraAngles = glm::vec3(0.0f);
};