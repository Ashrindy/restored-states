#define EXPORT extern "C" __declspec(dllexport)
#include <Pch.h>

bool drift = true;
bool jumpDash = true;
bool doubleJumpDash = true;
bool lightDash = true;
bool stompBounce = true;
bool stompBounceLightning = true;
bool stompBounceLightningFix = true;
bool legacyControls = false;

EXPORT void Init()
{
	mINI::INIFile file("config.ini");
	mINI::INIStructure ini;
	if (!file.read(ini))
		FatalExit(-1);

	auto& states = ini["States"];

	drift = states["drift"] == "True";
	jumpDash = states["jumpDash"] == "True";
	doubleJumpDash = states["doubleJumpDash"] == "True";
	lightDash = states["lightDash"] == "True";
	stompBounce = states["stompBounce"] == "True";
	stompBounceLightning = states["stompBounceLightning"] == "True";
	stompBounceLightningFix = states["stompBounceLightningFix"] == "True";
	legacyControls = states["legacyControls"] == "True";
}

EXPORT void PostInit()
{
}

EXPORT void OnFrame()
{

}