﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"

namespace FVoxelUtilities
{
	struct CForceInitializable
	{
		template<typename T>
		auto Requires() -> decltype(T(ForceInit));
	};

	template<typename T>
	static constexpr bool CanMakeSafe =
		!TIsTSharedRef_V<T> &&
		!TIsDerivedFrom<T, UObject>::Value;

	template<typename T, typename = std::enable_if_t<CanMakeSafe<T>>>
	std::enable_if_t<!TModels<CForceInitializable, T>::Value, T> MakeSafe()
	{
		return T{};
	}
	template<typename T, typename = std::enable_if_t<CanMakeSafe<T>>>
	std::enable_if_t<TModels<CForceInitializable, T>::Value, T> MakeSafe()
	{
		return T(ForceInit);
	}

	template<typename T>
	T MakeZeroed()
	{
		T Value;
		FMemory::Memzero(Value);
		return Value;
	}
}