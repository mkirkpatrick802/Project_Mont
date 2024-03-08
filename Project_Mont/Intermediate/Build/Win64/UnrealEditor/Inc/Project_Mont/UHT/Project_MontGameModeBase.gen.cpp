// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Project_Mont/Project_MontGameModeBase.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeProject_MontGameModeBase() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_AGameModeBase();
	PROJECT_MONT_API UClass* Z_Construct_UClass_AProject_MontGameModeBase();
	PROJECT_MONT_API UClass* Z_Construct_UClass_AProject_MontGameModeBase_NoRegister();
	UPackage* Z_Construct_UPackage__Script_Project_Mont();
// End Cross Module References
	void AProject_MontGameModeBase::StaticRegisterNativesAProject_MontGameModeBase()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AProject_MontGameModeBase);
	UClass* Z_Construct_UClass_AProject_MontGameModeBase_NoRegister()
	{
		return AProject_MontGameModeBase::StaticClass();
	}
	struct Z_Construct_UClass_AProject_MontGameModeBase_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AProject_MontGameModeBase_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AGameModeBase,
		(UObject* (*)())Z_Construct_UPackage__Script_Project_Mont,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AProject_MontGameModeBase_Statics::Class_MetaDataParams[] = {
		{ "Comment", "/**\n * \n */" },
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "Project_MontGameModeBase.h" },
		{ "ModuleRelativePath", "Project_MontGameModeBase.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AProject_MontGameModeBase_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AProject_MontGameModeBase>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AProject_MontGameModeBase_Statics::ClassParams = {
		&AProject_MontGameModeBase::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009002ACu,
		METADATA_PARAMS(Z_Construct_UClass_AProject_MontGameModeBase_Statics::Class_MetaDataParams, UE_ARRAY_COUNT(Z_Construct_UClass_AProject_MontGameModeBase_Statics::Class_MetaDataParams))
	};
	UClass* Z_Construct_UClass_AProject_MontGameModeBase()
	{
		if (!Z_Registration_Info_UClass_AProject_MontGameModeBase.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AProject_MontGameModeBase.OuterSingleton, Z_Construct_UClass_AProject_MontGameModeBase_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AProject_MontGameModeBase.OuterSingleton;
	}
	template<> PROJECT_MONT_API UClass* StaticClass<AProject_MontGameModeBase>()
	{
		return AProject_MontGameModeBase::StaticClass();
	}
	AProject_MontGameModeBase::AProject_MontGameModeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AProject_MontGameModeBase);
	AProject_MontGameModeBase::~AProject_MontGameModeBase() {}
	struct Z_CompiledInDeferFile_FID_Users_mKirkpatrick_Documents_Project_Mont_Project_Mont_Source_Project_Mont_Project_MontGameModeBase_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_mKirkpatrick_Documents_Project_Mont_Project_Mont_Source_Project_Mont_Project_MontGameModeBase_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AProject_MontGameModeBase, AProject_MontGameModeBase::StaticClass, TEXT("AProject_MontGameModeBase"), &Z_Registration_Info_UClass_AProject_MontGameModeBase, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AProject_MontGameModeBase), 2430317674U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Users_mKirkpatrick_Documents_Project_Mont_Project_Mont_Source_Project_Mont_Project_MontGameModeBase_h_2139701764(TEXT("/Script/Project_Mont"),
		Z_CompiledInDeferFile_FID_Users_mKirkpatrick_Documents_Project_Mont_Project_Mont_Source_Project_Mont_Project_MontGameModeBase_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Users_mKirkpatrick_Documents_Project_Mont_Project_Mont_Source_Project_Mont_Project_MontGameModeBase_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
