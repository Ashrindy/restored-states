#include "Pch.h"
#include <ucsl/resources/effdb/v100.h>

template<typename T>
void WriteProtected(uintptr_t address, T value) {
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<T*>(address) = value;
	VirtualProtect(reinterpret_cast<void*>(address), sizeof(T), oldProtect, &oldProtect);
}

void doJumpDash(app::player::PlayerHsmContext* context) {
	if (auto* input = context->playerObject->GetComponent<hh::game::GOCInput>()) {
		if (auto* inputcomp = input->GetInputComponent()) {
			if (inputcomp->actionMonitors[12].state & 512) {
				context->playerObject->GetComponent<app::player::GOCPlayerHsm>()->hsm.ChangeState(13);
			}
		}
	}
}

void doLightDash(app::player::PlayerHsmContext* context) {
	if (auto* input = context->playerObject->GetComponent<hh::game::GOCInput>()) {
		if (auto* inputcomp = input->GetInputComponent()) {
			if (inputcomp->actionMonitors[2].state & 512) {
				context->playerObject->GetComponent<app::player::GOCPlayerHsm>()->hsm.ChangeState(25);
			}
		}
	}
}

static const float driftZone = .075f;

void doDrift(app::player::PlayerHsmContext* context) {
	if (auto* input = context->playerObject->GetComponent<hh::game::GOCInput>()) {
		if (auto* inputcomp = input->GetInputComponent()) {
			if (inputcomp->actionMonitors[11].state & 256 && (abs(inputcomp->axisMonitors[0].state) > driftZone || abs(inputcomp->axisMonitors[1].state) > driftZone)) {
				context->playerObject->GetComponent<app::player::GOCPlayerHsm>()->hsm.ChangeState(106);
			}
		}
	}
}

FUNCTION_PTR(int64_t, __fastcall, shouldDSurf, 0x140704D80, app::player::PlayerHsmContext*, char);

void doDefault(app::player::PlayerHsmContext* context) {
	if (auto* input = context->playerObject->GetComponent<hh::game::GOCInput>()) {
		if (auto* inputcomp = input->GetInputComponent()) {
			shouldDSurf(context, 0);
			if (!(inputcomp->actionMonitors[11].state & 256))
				context->playerObject->GetComponent<app::player::GOCPlayerHsm>()->hsm.ChangeState(1);
		}
	}
}

HOOK(bool, __fastcall, JumpStep, 0x14A7C1140, app::player::StateJump* self, app::player::PlayerHsmContext* context, float deltaTime) {
	auto res = originalJumpStep(self, context, deltaTime);
	if (jumpDash)
		doJumpDash(context);
	if (lightDash)
		doLightDash(context);
	return res;
}

HOOK(bool, __fastcall, DoubleJumpStep, 0x14A7ADE20, app::player::StateDoubleJump* self, app::player::PlayerHsmContext* context, float deltaTime) {
	auto res = originalDoubleJumpStep(self, context, deltaTime);
	if (doubleJumpDash)
		doJumpDash(context);
	if (lightDash)
		doLightDash(context);
	return res;
}

HOOK(bool, __fastcall, RunStep, 0x1406C7760, app::player::StateRun* self, app::player::PlayerHsmContext* context, float deltaTime) {
	auto res = originalRunStep(self, context, deltaTime);
	if (drift)
		doDrift(context);
	if(lightDash)
		doLightDash(context);
	return res;
}

HOOK(bool, __fastcall, DriftStep, 0x1406AF920, app::player::StateDrift* self, app::player::PlayerHsmContext* context, float deltaTime) {
	auto res = originalDriftStep(self, context, deltaTime);
	if (drift)
		doDefault(context);
	return res;
}

HOOK(void, __fastcall, BindMaps, 0x140A2FEC0, hh::game::GameManager* gameManager, hh::hid::InputMapSettings* inputSettings) {
	if (drift) {
		if (legacyControls)
			inputSettings->BindActionMapping("PlayerDrift", 0x10010u);
		else
			inputSettings->BindActionMapping("PlayerDrift", 0x10006u);
		inputSettings->BindActionMapping("PlayerDrift", 0x200e1u);
	}
	if (jumpDash || doubleJumpDash) {
		if (legacyControls)
			inputSettings->BindActionMapping("PlayerJumpDash", 0x10010u);
		else
			inputSettings->BindActionMapping("PlayerJumpDash", 0x10004u);
		inputSettings->BindActionMapping("PlayerJumpDash", 0x40002u);
	}
	if (lightDash)
		inputSettings->BindActionMapping("PlayerLightDash", 0x2001du);
	originalBindMaps(gameManager, inputSettings);
}

static int bounce = -1;

bool StateStandUpdate(app::player::StateStand* self, app::player::PlayerHsmContext* context, float deltaTime) {
	if(lightDash)
		doLightDash(context);
	if(stompBounceLightning)
		bounce = -1;
	return false;
}

static char addMonitors = 0;

HOOK(void, __fastcall, PlayerAddCallback, 0x140608C60, app::player::Player* self, hh::game::GameManager* gameManager) {
	addMonitors += drift + (jumpDash||doubleJumpDash);
	if (lightDash || stompBounceLightning)
		WriteProtected<void*>(0x14125EF88, &StateStandUpdate);
	if (lightDash)
		WriteProtected<char>(0x14125BEF1, 'd');
	/*if (drift)
		WriteProtected<char>(0x141258619, 'd');*/
	WriteProtected<char>(0x140608F24, (char)11 + addMonitors);
	if(stompBounceLightning)
		WriteProtected<char>(0x14125FC29, 'd');
	originalPlayerAddCallback(self, gameManager);
	if (drift || (jumpDash || doubleJumpDash))
		if (auto* inputcomp = self->GetComponent<hh::game::GOCInput>()->GetInputComponent()) {
			if(drift)
				inputcomp->MonitorActionMapping("PlayerDrift", 11, 2);
			if(jumpDash || doubleJumpDash)
				inputcomp->MonitorActionMapping("PlayerJumpDash", 12, 2);
		}
}

static ucsl::resources::master_level::v0::ResourceData sounds[] = {
	{ "sound/shadow_sound/se_miller_character_shadow_extra.acb", nullptr, 0}
};

HOOK(hh::fnd::ManagedResource*, __fastcall, GetResourceEx, 0x140C090C0, hh::fnd::ResourceManager* self, const char* name, const hh::fnd::ResourceTypeInfo* resourceTypeInfo, const hh::fnd::ResourceManager::GetResourceExInfo* info) {
	if (drift) {
		if (strcmp(resourceTypeInfo->pName, "ResAnimator") == 0) {
			if (strcmp(name, "chr_shadow") == 0) {
				hh::anim::ResAnimator* animator = static_cast<hh::anim::ResAnimator*>(originalGetResourceEx(self, name, resourceTypeInfo, info));
				auto* binaryData = animator->binaryData;
				for (int i = 0; i < binaryData->clipCount; i++) {
					auto& clip = binaryData->clips[i];
					if (strcmp(clip.animationSettings.resourceName, "chr_shadow@drift_l_loop") == 0)
						clip.animationSettings.resourceName = "chr_shadow@drift_l_loop_fixed";
					else if (strcmp(clip.animationSettings.resourceName, "chr_shadow@drift_r_loop") == 0)
						clip.animationSettings.resourceName = "chr_shadow@drift_r_loop_fixed";
				}
			}
		}
	}
	return originalGetResourceEx(self, name, resourceTypeInfo, info);
}

static hh::fnd::ResourceLoader* resourceLoader;

HOOK(int64_t, __fastcall, GameModeBootInit, 0x1475455B0, int64_t self) {
	resourceLoader = hh::fnd::ResourceLoader::Create(hh::fnd::MemoryRouter::GetModuleAllocator());
	if (drift)
		resourceLoader->LoadPackfile2("shadow_fixes.pac");
	return originalGameModeBootInit(self);
}

HOOK(void, __fastcall, EffectLoad, 0x140AAA600, hh::eff::ResEffect* self, void* data, size_t size) {
	if (stompBounceLightningFix) {
		if (strcmp(self->GetName(), "ec_sd_stomp_end01_third_lightning01") == 0)
			data = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::eff::ResEffect>("ec_sd_stomp_end01_third_lightning01_fixed")->unpackedBinaryData;
		if (strcmp(self->GetName(), "ec_sd_stomp_end01_third_lightning02") == 0)
			data = hh::fnd::ResourceManager::GetInstance()->GetResource<hh::eff::ResEffect>("ec_sd_stomp_end01_third_lightning02_fixed")->unpackedBinaryData;
	}
	originalEffectLoad(self, data, size);
}

HOOK(void, __fastcall, MasterLevelLoad, 0x140A2D080, app::level::ResMasterLevel* self, void* data, size_t size) {
	auto* binaryData = static_cast<ucsl::resources::master_level::v0::MasterLevelData*>(data);
	for (int i = 0; i < binaryData->levelCount; i++) {
		auto* level = binaryData->levels[i];
		if (strcmp(level->name, "shadow_sound") == 0) {
			ucsl::resources::master_level::v0::ResourceData** resources = static_cast<ucsl::resources::master_level::v0::ResourceData**>(hh::fnd::MemoryRouter::GetModuleAllocator()->Alloc(sizeof(ucsl::resources::master_level::v0::ResourceData*) * (level->resourceCount + ARRAYSIZE(sounds)), 8));
			for (int x = 0; x < level->resourceCount; x++)
				resources[x] = level->resources[x];
			for (int x = 0; x < ARRAYSIZE(sounds); x++)
				resources[x + level->resourceCount] = &sounds[x];
			level->resourceCount += ARRAYSIZE(sounds);
			level->resources = resources;
			break;
		}
	}
	originalMasterLevelLoad(self, data, size);
}

HOOK(bool, __fastcall, SlidingEnter, 0x1406CBB70, app::player::StateSliding* self, app::player::PlayerHsmContext* context, int32_t previousState) {
	auto res = originalSlidingEnter(self, context, previousState);
	if (!legacyControls)
		if (drift)
			doDrift(context);
	return res;
}

HOOK(bool, __fastcall, BounceJumpEnter, 0x1406C06E0, app::player::StateBounceJump* self, app::player::PlayerHsmContext* context, int32_t previousState) {
	if (stompBounceLightning)
		bounce++;
	auto res = originalBounceJumpEnter(self, context, previousState);
	if (bounce == 2)
		context->gocPlayerHsm->hsm.ChangeState(20);
	return res;
}

HOOK(bool, __fastcall, StompingLandEnter, 0x1406D0430, app::player::StateStompingLand* self, app::player::PlayerHsmContext* context, int32_t previousState) {
	if (stompBounce) {
		reinterpret_cast<float*>(self)[57] = 0.08f;
		if (stompBounceLightning) {
			reinterpret_cast<float*>(self)[56] = bounce;
			if (bounce > 1)
				bounce = -2;
		}
		else
			reinterpret_cast<float*>(self)[56] = 0;
	}
	auto res = originalStompingLandEnter(self, context, previousState);
	return res;
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		INSTALL_HOOK(JumpStep);
		INSTALL_HOOK(StompingLandEnter);
		INSTALL_HOOK(BounceJumpEnter);
		INSTALL_HOOK(DoubleJumpStep);
		INSTALL_HOOK(MasterLevelLoad);
		INSTALL_HOOK(EffectLoad);
		INSTALL_HOOK(SlidingEnter);
		if (drift || lightDash) {
			INSTALL_HOOK(RunStep);
			INSTALL_HOOK(PlayerAddCallback);
		}
		if (drift) {
			INSTALL_HOOK(DriftStep);
			INSTALL_HOOK(BindMaps);
			INSTALL_HOOK(GetResourceEx);
			INSTALL_HOOK(GameModeBootInit);
		}
		break;
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
