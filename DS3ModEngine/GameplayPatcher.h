#pragma once

#include "MinHook\include\MinHook.h"

BOOL ApplyGameplayPatches();

BOOL ApplyMiscPatches();

BOOL ApplyShadowMapResolutionPatches(int dirSize, int atlasSize, int pointSize, int spotSize);