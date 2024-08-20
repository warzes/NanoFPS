#include "app.h"

#include "raylib.h"
#include "raymath.h"

#include "imgui/rlImGui.h"

#include "assets/fonts/softball_gold_ttf.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "raylib.lib" )
#	pragma comment( lib, "Winmm.lib" )
#endif

//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	// Window stuff
	InitWindow(1280, 720, "Editor");
	SetWindowMinSize(640, 480);
	SetWindowState(FLAG_WINDOW_RESIZABLE);

	SetExitKey(KEY_NULL);

	// Set random seed based on system time;
	using std::chrono::high_resolution_clock;
	SetRandomSeed((int)high_resolution_clock::now().time_since_epoch().count());

	rlImGuiSetup(true);

	// ImGUI styling
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;
	io.FontDefault = io.Fonts->AddFontFromMemoryTTF((void*)Softball_Gold_ttf, Softball_Gold_ttf_len, 24, &config, io.Fonts->GetGlyphRangesCyrillic());

	io.ConfigFlags = ImGuiConfigFlags_NavNoCaptureKeyboard;

	rlImGuiReloadFonts();

#ifdef DEBUG
	SetTraceLogLevel(LOG_WARNING);
#else
	SetTraceLogLevel(LOG_ERROR);
#endif

	App::Get()->ChangeEditorMode(App::Mode::PLACE_TILE);
	App::Get()->NewMap(100, 5, 100);

	// Main loop
	SetTargetFPS(60);
	while (!App::Get()->IsQuitting())
	{
		App::Get()->Update();
	}

	rlImGuiShutdown();

	CloseRaylibWindow();

	return 0;

	return 0;
}
//-----------------------------------------------------------------------------