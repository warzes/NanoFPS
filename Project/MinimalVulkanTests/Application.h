#pragma once

#include "Timer.h"

struct WindowCreateInfo final
{
	uint32_t width{ 1600 };
	uint32_t height{ 900 };
	bool resize = true;
};

struct ApplicationCreateInfo final
{
	WindowCreateInfo window{};
};

class Application
{
public:
	Application();
	virtual ~Application() = default;

	void Run();

	virtual ApplicationCreateInfo GetConfig() = 0;
	virtual bool Start() = 0;
	virtual void Shutdown() = 0;
	virtual void FixedUpdate(float deltaTime) = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Frame(float deltaTime) = 0;

	void Print(const std::string& text);
	void Warning(const std::string& text);
	void Error(const std::string& text);
	void Fatal(const std::string& text);

	void Exit();

	[[nodiscard]] float GetFramerate() const;

	[[nodiscard]] uint32_t GetWindowWidth() const;
	[[nodiscard]] uint32_t GetWindowHeight() const;

	[[nodiscard]] std::string GetExtension(const std::string& uri);

	[[nodiscard]] std::string ReadFileString(const std::filesystem::path& path);
	[[nodiscard]] std::vector<uint8_t> ReadChunk(const std::filesystem::path& path, size_t offset, size_t count);
	[[nodiscard]] std::vector<uint8_t> ReadFileBinary(const std::filesystem::path& path);

private:
	friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM) noexcept;

	bool start();
	bool createWindow(const ApplicationCreateInfo& createInfo);
	void shutdown();
	void fixedUpdate(float deltaTime);
	void beginFrame();
	void present();
	void pollEvent();
	void update(float deltaTime);

	std::array<double, 64> m_framerateArray = { {0.0} };
	uint64_t               m_framerateTick{ 0 };

	HINSTANCE              m_handleInstance{ nullptr };
	HWND                   m_hwnd{ nullptr };
	MSG                    m_msg{};
	uint32_t               m_windowWidth{ 0 };
	uint32_t               m_windowHeight{ 0 };

	bool                   m_minimized{ false };
	bool                   m_requestedExit{ false };
};