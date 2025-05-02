#include "Pch.h"

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
			if (inputcomp->actionMonitors[1].state & 512) {
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

void doDrift(app::player::PlayerHsmContext* context) {
	if (auto* input = context->playerObject->GetComponent<hh::game::GOCInput>()) {
		if (auto* inputcomp = input->GetInputComponent()) {
			if (inputcomp->actionMonitors[11].state & 512) {
				/*if (context->blackboardStatus->stateFlags.test(app::player::BlackboardStatus::StateFlag::BOOST))
					context->playerObject->GetComponent<app::player::GOCPlayerHsm>()->hsm.ChangeState(107);
				else*/
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
	return res;
}

HOOK(bool, __fastcall, DoubleJumpStep, 0x14A7ADE20, app::player::StateDoubleJump* self, app::player::PlayerHsmContext* context, float deltaTime) {
	auto res = originalDoubleJumpStep(self, context, deltaTime);
	if (doubleJumpDash)
		doJumpDash(context);
	return res;
}

HOOK(bool, __fastcall, RunStep, 0x1406C7760, app::player::StateRun* self, app::player::PlayerHsmContext* context, float deltaTime) {
	auto res = originalRunStep(self, context, deltaTime);
	if(lightDash)
		doLightDash(context);
	if(drift)
		doDrift(context);
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
		inputSettings->BindActionMapping("PlayerDrift", 0x1000Du);
		inputSettings->BindActionMapping("PlayerDrift", 0x200e1u);

		originalBindMaps(gameManager, inputSettings);
	}
}

bool StateStandUpdate(app::player::StateStand* self, app::player::PlayerHsmContext* context, float deltaTime) {
	doLightDash(context);
	return false;
}

HOOK(void, __fastcall, PlayerAddCallback, 0x140608C60, app::player::Player* self, hh::game::GameManager* gameManager) {
	if(lightDash)
		WriteProtected<void*>(0x14125EF88, &StateStandUpdate);
	if(drift)
		WriteProtected<char>(0x140608F24, (char)12);
	originalPlayerAddCallback(self, gameManager);
	if (drift)
		if (auto* inputcomp = self->GetComponent<hh::game::GOCInput>()->GetInputComponent())
			inputcomp->MonitorActionMapping("PlayerDrift", 11, 2);
}

HOOK(hh::fnd::ManagedResource*, __fastcall, GetResourceEx, 0x140C090C0, hh::fnd::ResourceManager* self, const char* name, const hh::fnd::ResourceTypeInfo* resourceTypeInfo, const hh::fnd::ResourceManager::GetResourceExInfo* info) {
	if (drift) {
		if (strcmp(resourceTypeInfo->pName, "ResAnimator") == 0) {
			if (strcmp(name, "chr_shadow") == 0) {
				hh::anim::ResAnimator* animator = static_cast<hh::anim::ResAnimator*>(originalGetResourceEx(self, name, resourceTypeInfo, info));
				auto* binaryData = animator->binaryData;
				for (int i = 0; i < binaryData->clipCount; i++) {
					auto& clip = binaryData->clips[i];
					if (strcmp(clip.animationSettings.resourceName, "chr_shadow@drift_l_loop") == 0) {
						clip.animationSettings.resourceName = "chr_shadow@drift_l_loop_fixed";
					}
					else if (strcmp(clip.animationSettings.resourceName, "chr_shadow@drift_r_loop") == 0) {
						clip.animationSettings.resourceName = "chr_shadow@drift_r_loop_fixed";
					}
				}
			}
		}
	}
	return originalGetResourceEx(self, name, resourceTypeInfo, info);
}

HOOK(int64_t, __fastcall, GameModeBootInit, 0x1475455B0, int64_t self) {
	if (drift) {
		auto* resourceLoader = hh::fnd::ResourceLoader::Create(hh::fnd::MemoryRouter::GetModuleAllocator());
		resourceLoader->LoadPackfile2("shadow_fixes.pac");
	}
	return originalGameModeBootInit(self);
}

BOOL WINAPI DllMain(_In_ HINSTANCE hInstance, _In_ DWORD reason, _In_ LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		INSTALL_HOOK(JumpStep);
		INSTALL_HOOK(DoubleJumpStep);
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
