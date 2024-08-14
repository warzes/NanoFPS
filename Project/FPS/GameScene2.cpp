#include "GameScene.h"
#include "GameLua.h"

extern std::unique_ptr<Scene> scene;

struct QuakeModel
{
	std::vector<MapData::Brush> meshes;
	std::vector<int> materialIDs;
};

const float inverseScale = 32.0f;

QuakeModel readModelEntity(const qformats::map::SolidEntityPtr& ent)
{
	QuakeModel qm;

	for (const auto& b : ent->GetClippedBrushes())
	{
		for (const auto& p : b.GetFaces())
		{
			//if (p->noDraw)
			//{
			//	continue;
			//}
			p->UpdateNormals();
			int i = 0;
			int i_uv = 0;

			//auto mesh = Mesh{ 0 };
			auto vertices = p->GetVertices();
			auto indices = p->GetIndices();

			for (int v = vertices.size() - 1; v >= 0; v--)
			{
				//mesh.vertices[i] = -vertices[v].point[0] / inverseScale;
				//mesh.tangents[i] = vertices[v].tangent[0];
				//mesh.normals[i] = vertices[v].normal[0];
				i++;
				//mesh.vertices[i] = vertices[v].point[2] / inverseScale;
				//mesh.tangents[i] = vertices[v].tangent[2];
				//mesh.normals[i] = -vertices[v].normal[2];
				i++;
				//mesh.vertices[i] = vertices[v].point[1] / inverseScale;
				//mesh.tangents[i] = vertices[v].tangent[1];
				//mesh.normals[i] = -vertices[v].normal[1];

				i++;
				//mesh.texcoords[i_uv++] = vertices[v].uv[0];
				//mesh.texcoords[i_uv++] = vertices[v].uv[1];
			}
			
			//qm.meshes.push_back(mesh);
			//qm.materialIDs.push_back(p->TextureID());
		}
	}

	/*qm.model.meshCount = qm.meshes.size();
	qm.model.meshes = (Mesh*)MemAlloc(qm.model.meshCount * sizeof(Mesh));

	for (int m = 0; m < qm.meshes.size(); m++)
	{
		qm.model.meshes[m] = qm.meshes[m];
		int matID = qm.materialIDs[m];
		qm.model.meshMaterial[m] = matID;
	}*/
	return qm;
}

void LoadEntities2(const std::string& mapFilename)
{
#if 0
	//Print("Loading entities in map " + mapFilename);
	// TODO: parser map

	//qformats::map::QMap mapInstance;
	//qformats::wad::QuakeWadManager wadMgr;
	//std::vector<QuakeModel> models;

	//mapInstance = qformats::map::QMap();
	//mapInstance.LoadFile(mapFilename);

	//mapInstance.GenerateGeometry(true);

	//size_t clippedFacesTotal = 0;
	//for (const auto& se : mapInstance.GetSolidEntities())
	//{
	//	auto m = readModelEntity(se);
	//	models.push_back(m);
	//	clippedFacesTotal += se->StatsClippedFaces();
	//}

	//std::cout << "faces clipped: " << clippedFacesTotal << std::endl;


	//
	//// TODO: write map
	//
#else
	MapData::Map map;

	map.Entities.resize(5);

	// entity 0
	{
		auto& e = map.Entities[0];

	}

#endif
	LoadEntities(map);
}