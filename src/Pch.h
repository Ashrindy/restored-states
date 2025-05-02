#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#undef CreateService
#undef max
#undef min

#include <detours.h>
#include <d3d11.h>

#include <cstdint>
#include <miller-sdk.h>

#include <utilities/Helpers.h>

#include <ini.h>

#ifndef PCH_H
#define PCH_H
extern bool drift;
extern bool jumpDash;
extern bool doubleJumpDash;
extern bool lightDash;
#endif