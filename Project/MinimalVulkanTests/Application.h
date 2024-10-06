#pragma once

void Print(const std::string& text);
void Warning(const std::string& text);
void Error(const std::string& text);
void Fatal(const std::string& text);

struct ApplicationCreateInfo final
{
	struct Window
	{
		int width = 1600;
		int height = 900;
		std::string_view title = "Vulkan Lesson";
	} window;
};

class Application
{
public:
	Application();
	virtual ~Application() = default;

	void Run();

	virtual ApplicationCreateInfo GetConfig() = 0;
	virtual bool Init() = 0;
	virtual void Close() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;

	void Print(const std::string& text);
	void Warning(const std::string& text);
	void Error(const std::string& text);
	void Fatal(const std::string& text);

	[[nodiscard]] int GetWindowWidth() const;
	[[nodiscard]] int GetWindowHeight() const;

	[[nodiscard]] double GetDeltaTime() const;

	[[nodiscard]] std::optional<std::vector<char>> ReadFile(const std::filesystem::path& filename);

	void Exit();

	GLFWwindow* GetWindow() { return m_window; }
private:
	bool init();
	void close();
	void update();
	void draw();
	bool shouldClose() const;

	GLFWwindow* m_window{ nullptr };
	int         m_windowWidth{ 0 };
	int         m_windowHeight{ 0 };
	double      m_startTime{ 0.0 };
	double      m_deltaTime{ 0.0f };
	bool        m_isExit{ false };
};