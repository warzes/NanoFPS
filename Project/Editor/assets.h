#pragma once

#include "raylib.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <filesystem>
namespace fs = std::filesystem;

//A repository that caches all loaded resources and their file paths.
//It is implemented as a singleton with a static interface. 
//(This circumvents certain limitations regarding static members and allows the constructor to be called automatically when the first method is called.)
class Assets
{
public:
	//A RAII wrapper for a Raylib Texture (unloads on destruction)
	class TexHandle
	{
	public:
		inline TexHandle(Texture2D texture, fs::path path) { _texture = texture; _path = path; }
		inline ~TexHandle() { UnloadTexture(_texture); }
		inline Texture2D GetTexture() const { return _texture; }
		inline fs::path GetPath() const { return _path; }
	private:
		Texture2D _texture;
		fs::path _path;
	};

	//A RAII wrapper for a Raylib Model (unloads on destruction)
	class ModelHandle
	{
	public:
		// Loads an .obj model from the given path and initializes a handle for it.
		ModelHandle(const fs::path path);
		~ModelHandle();
		inline Model GetModel() const { return _model; }
		inline fs::path GetPath() const { return _path; }
	private:
		Model _model;
		fs::path _path;
	};

	static std::shared_ptr<TexHandle>   GetTexture(fs::path path); //Returns a shared pointer to the cached texture at `path`, loading it if it hasn't been loaded.
	static std::shared_ptr<ModelHandle> GetModel(fs::path path);   //Returns a shared pointer to the cached model at `path`, loading it if it hasn't been loaded.

	static const Font& GetFont(); //Returns the default application font (dejavu.fnt)
	static const Shader& GetMapShader(bool instanced); //Returns the shader used to render tiles
	static const Shader& GetSpriteShader(); // Returns the shader used to render billboards
	static const Model& GetEntSphere(); //Returns the sphere that represents entities visually
	static const Mesh& GetSpriteQuad();
	static Texture GetMissingTexture();
protected:
	//Asset caches that hold weak references to all the loaded textures and models
	std::map<fs::path, std::weak_ptr<TexHandle>>   _textures;
	std::map<fs::path, std::weak_ptr<ModelHandle>> _models;

	//Assets that are alive the whole application
	Font _font; //Default application font (dejavu.fnt)
	Texture2D _missingTexture; //Texture to display when the texture file to be loaded isn't found
	Model _entSphere; //The sphere that represents entities visually
	Shader _mapShader; //The non-instanced version that is used to render tiles outside of the map itself
	Shader _mapShaderInstanced; //The instanced version is used to render the tiles
	Shader _spriteShader;
	Mesh _spriteQuad;
private:
	Assets();
	~Assets();
	static Assets* _Get();
};