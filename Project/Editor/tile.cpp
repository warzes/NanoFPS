#include "tile.h"

#include "cppcodec/base64_default_rfc4648.hpp"

#include <assert.h>
#include <iostream>
#include <algorithm>
#include <vector>

#include "assets.h"
#include "app.h"
#include "map_man.h"
#include "c_helpers.h"

TileGrid::TileGrid()
	: TileGrid(nullptr, 0, 0, 0)
{
}

TileGrid::TileGrid(MapMan* mapMan, size_t width, size_t height, size_t length)
	: TileGrid(mapMan, width, height, length, TILE_SPACING_DEFAULT, Tile{ NO_MODEL, 0, NO_TEX, 0 })
{
}

TileGrid::TileGrid(MapMan* mapMan, size_t width, size_t height, size_t length, float spacing, Tile fill)
	: Grid<Tile>(width, height, length, spacing, fill)
{
	_mapMan = mapMan;
	_batchFromY = 0;
	_batchToY = height - 1;
	_batchPosition = Vector3Zero();
	_model = nullptr;
	_regenBatches = true;
	_regenModel = true;
	_modelCulled = false;
}

TileGrid::~TileGrid()
{
}

Tile TileGrid::GetTile(int i, int j, int k) const
{
	return GetCel(i, j, k);
}

Tile TileGrid::GetTile(int flatIndex) const
{
	return _grid[flatIndex];
}

void TileGrid::SetTile(int i, int j, int k, const Tile& tile)
{
	SetCel(i, j, k, tile);
	_regenBatches = true;
	_regenModel = true;
}

void TileGrid::SetTile(int flatIndex, const Tile& tile)
{
	_grid[flatIndex] = tile;
	_regenBatches = true;
	_regenModel = true;
}

void TileGrid::SetTileRect(int i, int j, int k, int w, int h, int l, const Tile& tile)
{
	assert(i >= 0 && j >= 0 && k >= 0);
	assert(i + w <= int(_width) && j + h <= int(_height) && k + l <= int(_length));
	for (int y = j; y < j + h; ++y)
	{
		for (int z = k; z < k + l; ++z)
		{
			size_t base = FlatIndex(0, y, z);
			for (int x = i; x < i + w; ++x)
			{
				_grid[base + x] = tile;
			}
		}
	}
	_regenBatches = true;
	_regenModel = true;
}

void TileGrid::CopyTiles(int i, int j, int k, const TileGrid& src, bool ignoreEmpty)
{
	assert(i >= 0 && j >= 0 && k >= 0);
	int xEnd = Min(i + int(src._width), int(_width));
	int yEnd = Min(j + int(src._height), int(_height));
	int zEnd = Min(k + int(src._length), int(_length));
	for (int z = k; z < zEnd; ++z)
	{
		for (int y = j; y < yEnd; ++y)
		{
			size_t ourBase = FlatIndex(0, y, z);
			size_t theirBase = src.FlatIndex(0, y - j, z - k);
			for (int x = i; x < xEnd; ++x)
			{
				const Tile& tile = src._grid[theirBase + (x - i)];
				if (!ignoreEmpty || tile)
				{
					_grid[ourBase + x] = tile;
				}
			}
		}
	}
	_regenBatches = true;
	_regenModel = true;
}

void TileGrid::UnsetTile(int i, int j, int k)
{
	_grid[FlatIndex(i, j, k)].shape = NO_MODEL;
	_regenBatches = true;
	_regenModel = true;
}

TileGrid TileGrid::Subsection(int i, int j, int k, int w, int h, int l) const
{
	assert(i >= 0 && j >= 0 && k >= 0);
	assert(i + w <= int(_width) && j + h <= int(_height) && k + l <= int(_length));

	TileGrid newGrid(_mapMan, w, h, l);

	SubsectionCopy(i, j, k, w, h, l, newGrid);

	newGrid._regenBatches = true;

	return newGrid;
}

void TileGrid::_RegenBatches(Vector3 position, int fromY, int toY)
{
	if (!_mapMan) return;

	_drawBatches.clear();
	_batchFromY = fromY;
	_batchToY = toY;
	_batchPosition = position;
	_regenBatches = false;

	const size_t layerArea = _width * _length;
	// Create a hash map of dynamic arrays for each combination of texture and mesh
	for (int y = fromY; y <= toY; ++y)
	{
		for (size_t t = y * layerArea; t < (y + 1) * layerArea; ++t)
		{
			const Tile& tile = _grid[t];
			if (tile)
			{
				// Calculate world space matrix for the tile
				Vector3 gridPos = UnflattenIndex(t);
				Vector3 worldPos = Vector3Add(position, GridToWorldPos(gridPos, true));
				Matrix rotMatrix = TileRotationMatrix(tile);
				Matrix matrix = MatrixMultiply(rotMatrix, MatrixTranslate(worldPos.x, worldPos.y, worldPos.z));

				const Model& shape = _mapMan->ModelFromID(tile.shape);
				for (int m = 0; m < shape.meshCount; ++m)
				{
					// Add the tile's transform to the instance arrays for each mesh
					auto pair = std::make_pair(tile.texture, &shape.meshes[m]);
					if (_drawBatches.find(pair) == _drawBatches.end())
					{
						// Put in a vector for this pair if there hasn't been one already
						_drawBatches[pair] = std::vector<Matrix>();
					}
					_drawBatches[pair].push_back(matrix);
				}
			}
		}
	}
}

void TileGrid::Draw(Vector3 position)
{
	Draw(position, 0, _height - 1);
}

void TileGrid::Draw(Vector3 position, int fromY, int toY)
{
	// std::cout << "TILE DRAWING IDs" << std::endl;
	if (!_mapMan) return;

	if (App::Get()->IsPreviewing())
	{
		DrawModel(GetModel(), position, 1.0f, WHITE);
	}
	else
	{
		if (_regenBatches || fromY != _batchFromY || toY != _batchToY || !Vector3Equals(position, _batchPosition))
		{
			_RegenBatches(position, fromY, toY);
		}

		Material tileMaterial = LoadMaterialDefault();
		tileMaterial.shader = Assets::GetMapShader(true);

		//Call DrawMeshInstanced for each combination of material and mesh.
		for (auto& [pair, matrices] : _drawBatches) {
			//Reusing the same material for everything and just changing the albedo map between batches
			Texture2D texture = _mapMan->TexFromID(pair.first);
			// std::cout << texture.id << std::endl;
			SetMaterialTexture(&tileMaterial, MATERIAL_MAP_ALBEDO, texture);
			DrawMeshInstanced(*pair.second, tileMaterial, matrices.data(), matrices.size());
		}

		//Free material w/o unloading its textures
		RL_FREE(tileMaterial.maps);
	}
}

std::string TileGrid::GetTileDataBase64() const
{
	std::vector<uint8_t> bin;
	bin.reserve(_grid.size() * sizeof(Tile));
	for (size_t i = 0; i < _grid.size(); ++i)
	{
		Tile savedTile = _grid[i];

		//Reinterpret each tile as a series of bytes and push them onto the vector.
		const char* tileBin = reinterpret_cast<const char*>(&savedTile);
		for (size_t b = 0; b < sizeof(Tile); ++b)
		{
			bin.push_back(tileBin[b]);
		}
	}

	return base64::encode(bin);
}

std::string TileGrid::GetOptimizedTileDataBase64() const
{
	std::vector<uint8_t> bin;
	bin.reserve(_grid.size() * sizeof(Tile));

	int runLength = 0;
	for (size_t i = 0; i < _grid.size(); ++i)
	{
		Tile savedTile = _grid[i];

		if (!savedTile && i < _grid.size() - 1)
		{
			// Blank tiles (except for the last tile in the grid) are represented as runs.
			++runLength;
		}
		else
		{
			if (runLength > 0)
			{
				//Insert a special tile signifying the number of empty tiles preceding this one.
				Tile runTile = { -runLength, runLength, -runLength, runLength };
				const char* tileBin = reinterpret_cast<const char*>(&runTile);
				for (size_t b = 0; b < sizeof(Tile); ++b)
				{
					bin.push_back(tileBin[b]);
				}
				runLength = 0;
			}

			//Reinterpret the tile as a series of bytes and push them onto the vector.
			const char* tileBin = reinterpret_cast<const char*>(&savedTile);
			for (size_t b = 0; b < sizeof(Tile); ++b)
			{
				bin.push_back(tileBin[b]);
			}
		}
	}

	return base64::encode(bin);
}

void TileGrid::SetTileDataBase64(std::string data)
{
	std::vector<uint8_t> bin = base64::decode(data);
	size_t gridIndex = 0;
	for (size_t b = 0; b < bin.size(); b += sizeof(Tile))
	{
		// Reinterpret groups of bytes as tiles and place them into the grid.
		Tile loadedTile = *(reinterpret_cast<Tile*>(&bin[b]));
		if (loadedTile.shape < 0)
		{
			// This tile represents a run of blank tiles
			int j = 0;
			for (j = 0; j < -loadedTile.shape; ++j)
			{
				_grid[gridIndex + j] = Tile{ NO_MODEL, 0, NO_TEX, 0 };
			}
			gridIndex += j;
		}
		else
		{
			_grid[gridIndex] = loadedTile;
			++gridIndex;
		}
	}
	std::cout << std::endl;
}

std::pair<std::vector<TexID>, std::vector<ModelID>> TileGrid::GetUsedIDs() const
{
	std::set<TexID> usedTexIDs;
	std::set<ModelID> usedModelIDs;
	for (const auto& tile : _grid)
	{
		if (tile)
		{
			usedTexIDs.insert(tile.texture);
			usedModelIDs.insert(tile.shape);
		}
	}

	//Convert the sets to vectors and return
	return std::make_pair(
		std::vector(usedTexIDs.begin(), usedTexIDs.end()),
		std::vector(usedModelIDs.begin(), usedModelIDs.end()));
}

Model* TileGrid::_GenerateModel(bool culling)
{
	if (!_mapMan) return nullptr;

	_RegenBatches(Vector3Zero(), 0, _height - 1);

	// Collects vertex data for one of the model's meshes
	// There is one mesh per texture in the model, which contains all of the geometry with said texture.
	struct DynMesh
	{
		std::vector<float> positions;
		std::vector<float> texCoords;
		std::vector<float> normals;
		std::vector<unsigned short> indices;
		int triCount; // Independent form indices count since some models may not have indices
	};

	std::vector<DynMesh> meshMap(_mapMan->GetNumTextures());

	for (auto& dynMesh : meshMap)
	{
		dynMesh.positions = std::vector<float>();
		dynMesh.texCoords = std::vector<float>();
		dynMesh.normals = std::vector<float>();
		dynMesh.indices = std::vector<unsigned short>();
		dynMesh.triCount = 0;
	}

	for (const auto& [pair, matrices] : _drawBatches)
	{
		Mesh& shape = *pair.second;
		DynMesh& mesh = meshMap[pair.first];
		//Generate vertex data for this tile.
		for (const Matrix& matrix : matrices)
		{
			// The index of the first vertex belonging to this shape.
			int vBase = mesh.positions.size() / 3;
			// Add vertex data
			for (int v = 0; v < shape.vertexCount; v++)
			{
				if (shape.vertices != NULL)
				{
					//Transform shape vertices into tile's orientation and position
					Vector3 vec = Vector3{ shape.vertices[v * 3], shape.vertices[v * 3 + 1], shape.vertices[v * 3 + 2] };
					vec = Vector3Transform(vec, matrix);
					mesh.positions.push_back(vec.x);
					mesh.positions.push_back(vec.y);
					mesh.positions.push_back(vec.z);
				}

				if (shape.normals != NULL)
				{
					//Transform normals by the tile's rotation, but not its position
					Matrix rotMatrix = matrix;
					rotMatrix.m12 = 0.0f;
					rotMatrix.m13 = 0.0f;
					rotMatrix.m14 = 0.0f;

					Vector3 norm = Vector3{ shape.normals[v * 3], shape.normals[v * 3 + 1], shape.normals[v * 3 + 2] };
					norm = Vector3Transform(norm, rotMatrix);
					mesh.normals.push_back(norm.x);
					mesh.normals.push_back(norm.y);
					mesh.normals.push_back(norm.z);
				}

				if (shape.texcoords != NULL)
				{
					//Tex coordinates are just copied into the aggregate mesh
					mesh.texCoords.push_back(shape.texcoords[v * 2]);
					mesh.texCoords.push_back(shape.texcoords[v * 2 + 1]);
				}
			}
			// Add face data
			if (shape.indices != NULL)
			{
				for (int tri = 0; tri < shape.triangleCount; ++tri)
				{
					// Vertex indices
					unsigned short v0Index = (unsigned short)(vBase + shape.indices[tri * 3 + 0]);
					unsigned short v1Index = (unsigned short)(vBase + shape.indices[tri * 3 + 1]);
					unsigned short v2Index = (unsigned short)(vBase + shape.indices[tri * 3 + 2]);

					// Do face culling
					if (culling)
					{
						// Vertices
						Vector3 v0 = Vector3{ mesh.positions[v0Index * 3 + 0], mesh.positions[v0Index * 3 + 1], mesh.positions[v0Index * 3 + 2] };
						Vector3 v1 = Vector3{ mesh.positions[v1Index * 3 + 0], mesh.positions[v1Index * 3 + 1], mesh.positions[v1Index * 3 + 2] };
						Vector3 v2 = Vector3{ mesh.positions[v2Index * 3 + 0], mesh.positions[v2Index * 3 + 1], mesh.positions[v2Index * 3 + 2] };

						// Figure out if this triangle is near the border of the grid cel so it can be culled
						Vector3 planeNormal = Vector3CrossProduct(
							Vector3Subtract(v1, v0),
							Vector3Subtract(v2, v0)
						);
						planeNormal = Vector3Normalize(planeNormal);

						float planeDistance = -Vector3DotProduct(planeNormal, v0);

						Vector3 tileGridPosition = WorldToGridPos(Vector3{ matrix.m12, matrix.m13, matrix.m14 });

						// Determine the direction of the tile neighboring this face
						int neighborX = (int)tileGridPosition.x;
						int neighborY = (int)tileGridPosition.y;
						int neighborZ = (int)tileGridPosition.z;
						if (Vector3Equals(planeNormal, Vector3{ +1.0f,  0.0f,  0.0f })) neighborX += 1;
						else if (Vector3Equals(planeNormal, Vector3{ -1.0f,  0.0f,  0.0f })) neighborX -= 1;
						else if (Vector3Equals(planeNormal, Vector3{ 0.0f,  0.0f, -1.0f })) neighborZ -= 1;
						else if (Vector3Equals(planeNormal, Vector3{ 0.0f,  0.0f, +1.0f })) neighborZ += 1;
						else if (Vector3Equals(planeNormal, Vector3{ 0.0f, +1.0f,  0.0f })) neighborY += 1;
						else if (Vector3Equals(planeNormal, Vector3{ 0.0f, -1.0f,  0.0f })) neighborY -= 1;
						else goto nocull;

						// Look at the neighboring tile's faces to determine whether to cull this triangle or not.
						if (neighborX >= 0 && neighborY >= 0 && neighborZ >= 0 && neighborX < _width && neighborY < _height && neighborZ < _length)
						{
							Tile neighborTile = GetTile(neighborX, neighborY, neighborZ);
							if (!neighborTile)
								goto nocull;

							Vector3 nWorldPos = GridToWorldPos(Vector3{ (float)neighborX, (float)neighborY, (float)neighborZ }, true);
							Matrix nRotMatrix = TileRotationMatrix(neighborTile);
							Matrix nMatrix = MatrixMultiply(nRotMatrix, MatrixTranslate(nWorldPos.x, nWorldPos.y, nWorldPos.z));

							Model neighborModel = _mapMan->ModelFromID(neighborTile.shape);
							for (int nm = 0; nm < neighborModel.meshCount; ++nm)
							{
								Mesh neighborMesh = neighborModel.meshes[nm];
								for (int nt = 0; nt < neighborMesh.triangleCount; ++nt)
								{
									// Indices
									unsigned short nV0Index = neighborMesh.indices[nt * 3 + 0];
									unsigned short nV1Index = neighborMesh.indices[nt * 3 + 1];
									unsigned short nV2Index = neighborMesh.indices[nt * 3 + 2];

									// Vertices
									Vector3 nV0 = Vector3{ neighborMesh.vertices[nV0Index * 3 + 0], neighborMesh.vertices[nV0Index * 3 + 1], neighborMesh.vertices[nV0Index * 3 + 2] };
									nV0 = Vector3Transform(nV0, nMatrix);
									Vector3 nV1 = Vector3{ neighborMesh.vertices[nV1Index * 3 + 0], neighborMesh.vertices[nV1Index * 3 + 1], neighborMesh.vertices[nV1Index * 3 + 2] };
									nV1 = Vector3Transform(nV1, nMatrix);
									Vector3 nV2 = Vector3{ neighborMesh.vertices[nV2Index * 3 + 0], neighborMesh.vertices[nV2Index * 3 + 1], neighborMesh.vertices[nV2Index * 3 + 2] };
									nV2 = Vector3Transform(nV2, nMatrix);

									// Get neighbor's plane
									Vector3 nPlaneNormal = Vector3CrossProduct(
										Vector3Subtract(nV1, nV0),
										Vector3Subtract(nV2, nV0)
									);
									nPlaneNormal = Vector3Normalize(nPlaneNormal);

									float nPlaneDistance = -Vector3DotProduct(nPlaneNormal, nV0);

									// If the plane of the cullable triangle and the plane of the neighbor's triangle are in the same spot but opposite directions...
									if (FloatEquals(fabs(nPlaneDistance), fabs(planeDistance)) && FloatEquals(Vector3DotProduct(nPlaneNormal, planeNormal), -1.0f))
									{
										// Cull if all of the checked triangle's points correspond one of the neighbor's.
										if (
											(Vector3Equals(v0, nV0) || Vector3Equals(v0, nV1) || Vector3Equals(v0, nV2)) &&
											(Vector3Equals(v1, nV0) || Vector3Equals(v1, nV1) || Vector3Equals(v1, nV2)) &&
											(Vector3Equals(v2, nV0) || Vector3Equals(v2, nV1) || Vector3Equals(v2, nV2)))
										{
											goto cull;
										}
									}
								}
							}
						}
					}

				nocull:
					// Add indices, but with the offset of the current tile's vertices.
					mesh.indices.push_back(v0Index);
					mesh.indices.push_back(v1Index);
					mesh.indices.push_back(v2Index);
					++mesh.triCount;
				cull:
					continue;
				}
			}
		}
	}

	// Create Raylib mesh
	Model* model = SAFE_MALLOC(Model, 1);

	// Count the number of meshes (that aren't empty.)
	int numMeshes = 0;
	// We don't want to include any empty meshes, because that will cause an error in certain .gltf parsers.
	for (int i = 0; i < _mapMan->GetNumTextures(); ++i)
	{
		if (meshMap[i].triCount > 0) ++numMeshes;
	}

	model->materialCount = _mapMan->GetNumTextures();
	model->materials = SAFE_MALLOC(Material, model->materialCount);

	model->meshCount = numMeshes;
	model->meshes = SAFE_MALLOC(Mesh, model->meshCount);
	model->meshMaterial = SAFE_MALLOC(int, model->meshCount);

	model->transform = MatrixIdentity();
	model->bindPose = NULL;
	model->boneCount = 0;
	model->bones = NULL;

	// Initialize materials
	for (int m = 0; m < model->materialCount; ++m)
	{
		model->materials[m] = LoadMaterialDefault();
		model->materials[m].shader = Assets::GetMapShader(false);
		model->materials[m].maps[MATERIAL_MAP_ALBEDO].texture = _mapMan->TexFromID(m);
	}

	// Copy mesh data into Raylib mesh
	for (int i = 0, meshIndex = 0; i < model->materialCount; ++i)
	{
		DynMesh* dMesh = &meshMap[i];
		if (dMesh->triCount <= 0) continue;

		model->meshMaterial[meshIndex] = i;

		model->meshes[meshIndex] = Mesh{ 0 };
		model->meshes[meshIndex].vertexCount = dMesh->positions.size() / 3;
		model->meshes[meshIndex].triangleCount = dMesh->triCount;

		if (dMesh->positions.size() > 0)
		{
			model->meshes[meshIndex].vertices = SAFE_MALLOC(float, dMesh->positions.size());
			memcpy(model->meshes[meshIndex].vertices, dMesh->positions.data(), dMesh->positions.size() * sizeof(float));
		}
		if (dMesh->texCoords.size() > 0)
		{
			model->meshes[meshIndex].texcoords = SAFE_MALLOC(float, dMesh->texCoords.size());
			memcpy(model->meshes[meshIndex].texcoords, dMesh->texCoords.data(), dMesh->texCoords.size() * sizeof(float));
		}
		if (dMesh->normals.size() > 0)
		{
			model->meshes[meshIndex].normals = SAFE_MALLOC(float, dMesh->normals.size());
			memcpy(model->meshes[meshIndex].normals, dMesh->normals.data(), dMesh->normals.size() * sizeof(float));
		}
		if (dMesh->indices.size() > 0)
		{
			model->meshes[meshIndex].indices = SAFE_MALLOC(unsigned short, dMesh->indices.size());
			memcpy(model->meshes[meshIndex].indices, dMesh->indices.data(), dMesh->indices.size() * sizeof(unsigned short));
		}

		UploadMesh(&model->meshes[meshIndex], false);

		++meshIndex;
	}

	return model;
}

const Model TileGrid::GetModel()
{
	if (!_mapMan) return Model{};
	bool newCull = App::Get()->IsCullingEnabled();
	if (_regenModel || _model == nullptr || newCull != _modelCulled)
	{
		if (_model != nullptr)
		{
			UnloadModel(*_model);
		}
		_modelCulled = newCull;
		_model = _GenerateModel(_modelCulled);
		_regenModel = false;
	}

	return *_model;
}