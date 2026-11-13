// Alex Goldberg rights reserved

#include "Components/Appearance/HeirAppearanceComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DataAssets/Customisation/GodsentCustomisationDataAsset.h"
#include "DataAssets/Customisation/GodsentVoiceSetDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

namespace
{
	/// Reads a colour parameter from a material (leaves Out alone if it doesn't have it)
	void ReadColourParameter(const UMaterialInterface* Material, FName Parameter, FLinearColor& Out)
	{
		if (Material)
			Material->GetVectorParameterValue(FHashedMaterialParameterInfo(FMaterialParameterInfo(Parameter)), Out);
	}
}

UHeirAppearanceComponent::UHeirAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHeirAppearanceComponent::BeginPlay()
{
	Super::BeginPlay();

	// Remember the materials' own look before anything changes it
	DefaultAppearance = ReadAppearanceFromMesh();
	CurrentAppearance = DefaultAppearance;

	if (bApplyLineageAppearanceOnBeginPlay)
		ApplyLineageAppearance();
}

void UHeirAppearanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (VoiceLoadHandle.IsValid())
		VoiceLoadHandle->CancelHandle();

	VoiceLoadHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void UHeirAppearanceComponent::ApplyLineageAppearance()
{
	const UGodsentLineageSubsystem* Lineage = UGodsentLineageSubsystem::Get(this);

	if (Lineage && Lineage->GetHeirAppearance().bIsSet)
		ApplyAppearance(Lineage->GetHeirAppearance());
	else
		ApplyAppearance(DefaultAppearance);
}

void UHeirAppearanceComponent::ApplyAppearance(const FGodsentHeirAppearance& Appearance)
{
	CurrentAppearance = Appearance;

	if (UMaterialInstanceDynamic* MainArmour = GetSlotMaterial(MainArmourSlotName))
		MainArmour->SetVectorParameterValue(MetalColourParameter, Appearance.MainArmourColour);

	if (UMaterialInstanceDynamic* SecondaryArmour = GetSlotMaterial(SecondaryArmourSlotName))
	{
		SecondaryArmour->SetVectorParameterValue(MetalColourParameter, Appearance.SecondaryArmourColour);
		SecondaryArmour->SetVectorParameterValue(PatternColourParameter, Appearance.PatternColour);

		const UGodsentCustomisationDataAsset* Catalogue = UGodsentCustomisationDataAsset::GetCustomisationCatalogue(this);

		if (UTexture* PatternTexture = Catalogue ? Catalogue->LoadPatternTexture(Appearance.PatternIndex) : nullptr)
		{
			SecondaryArmour->SetTextureParameterValue(PatternTextureObjectParameter, PatternTexture);
			SecondaryArmour->SetTextureParameterValue(PatternTextureSampleParameter, PatternTexture);
		}
	}

	if (UMaterialInstanceDynamic* Cloth = GetSlotMaterial(ClothSlotName))
		Cloth->SetVectorParameterValue(ClothColourParameter, Appearance.ClothColour);

	LoadVoice(Appearance.VoiceIndex);
}

UGodsentVoiceSetDataAsset* UHeirAppearanceComponent::GetVoiceSet() const
{
	return VoiceSet;
}

UAudioComponent* UHeirAppearanceComponent::PlayVoiceLine(FGameplayTag VoiceLine)
{
	return VoiceSet ? PlaySound(VoiceSet->GetLine(VoiceLine)) : nullptr;
}

void UHeirAppearanceComponent::PlayRandomVoiceLine()
{
	// The new voice isn't in yet: play once it is (never the old voice)
	if (!LoadingVoicePath.IsNull())
	{
		bPlayRandomLineWhenLoaded = true;
		return;
	}

	if (VoiceSet)
		PlaySound(VoiceSet->GetRandomLine());
}

void UHeirAppearanceComponent::StopVoiceLine()
{
	if (UAudioComponent* Line = ActiveVoiceLine.Get())
		Line->Stop();

	ActiveVoiceLine.Reset();
}

USkeletalMeshComponent* UHeirAppearanceComponent::GetTargetMesh() const
{
	const AActor* Owner = GetOwner();

	if (!Owner)
		return nullptr;

	if (const ACharacter* Character = Cast<ACharacter>(Owner))
		return Character->GetMesh();

	return Owner->FindComponentByClass<USkeletalMeshComponent>();
}

UMaterialInstanceDynamic* UHeirAppearanceComponent::GetSlotMaterial(FName SlotName) const
{
	USkeletalMeshComponent* Mesh = GetTargetMesh();

	if (!Mesh)
		return nullptr;

	const int32 MaterialIndex = Mesh->GetMaterialIndex(SlotName);

	if (MaterialIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Appearance] %s has no material slot called %s"), *GetNameSafe(GetOwner()), *SlotName.ToString());
		return nullptr;
	}

	// Returns the existing dynamic material if there already is one
	return Mesh->CreateDynamicMaterialInstance(MaterialIndex);
}

FGodsentHeirAppearance UHeirAppearanceComponent::ReadAppearanceFromMesh() const
{
	FGodsentHeirAppearance Appearance;

	const UGodsentCustomisationDataAsset* Catalogue = UGodsentCustomisationDataAsset::GetCustomisationCatalogue(this);

	if (Catalogue)
		Appearance.VoiceIndex = Catalogue->DefaultVoiceIndex;

	const USkeletalMeshComponent* Mesh = GetTargetMesh();

	if (!Mesh)
		return Appearance;

	auto GetSlot = [Mesh](FName SlotName) -> UMaterialInterface*
	{
		const int32 MaterialIndex = Mesh->GetMaterialIndex(SlotName);
		return MaterialIndex != INDEX_NONE ? Mesh->GetMaterial(MaterialIndex) : nullptr;
	};

	const UMaterialInterface* MainArmour = GetSlot(MainArmourSlotName);
	const UMaterialInterface* SecondaryArmour = GetSlot(SecondaryArmourSlotName);
	const UMaterialInterface* Cloth = GetSlot(ClothSlotName);

	ReadColourParameter(MainArmour, MetalColourParameter, Appearance.MainArmourColour);
	ReadColourParameter(SecondaryArmour, MetalColourParameter, Appearance.SecondaryArmourColour);
	ReadColourParameter(SecondaryArmour, PatternColourParameter, Appearance.PatternColour);
	ReadColourParameter(Cloth, ClothColourParameter, Appearance.ClothColour);

	UTexture* PatternTexture = nullptr;

	if (SecondaryArmour && Catalogue &&
		SecondaryArmour->GetTextureParameterValue(FHashedMaterialParameterInfo(FMaterialParameterInfo(PatternTextureObjectParameter)), PatternTexture))
	{
		Appearance.PatternIndex = Catalogue->FindPatternIndex(PatternTexture);
	}

	return Appearance;
}

/// Loads the voice in the background (it's already in memory if the last look used it)
void UHeirAppearanceComponent::LoadVoice(int32 VoiceIndex)
{
	const UGodsentCustomisationDataAsset* Catalogue = UGodsentCustomisationDataAsset::GetCustomisationCatalogue(this);
	const TSoftObjectPtr<UGodsentVoiceSetDataAsset> NewVoiceSet = Catalogue ? Catalogue->GetVoiceSet(VoiceIndex) : TSoftObjectPtr<UGodsentVoiceSetDataAsset>();

	if (NewVoiceSet.IsNull())
	{
		VoiceSet = nullptr;
		LoadingVoicePath.Reset();
		return;
	}

	// Drop any voice still loading from an earlier pick
	if (VoiceLoadHandle.IsValid())
		VoiceLoadHandle->CancelHandle();

	VoiceLoadHandle.Reset();
	LoadingVoicePath.Reset();

	// Same voice as before: nothing to load
	if (VoiceSet && NewVoiceSet.ToSoftObjectPath() == FSoftObjectPath(VoiceSet.Get()))
		return;

	// Already in memory (picked earlier): use it straight away. The streamable manager would only
	// call back next frame, and a preview line played this frame would use the old voice.
	if (UGodsentVoiceSetDataAsset* LoadedVoiceSet = NewVoiceSet.Get())
	{
		VoiceSet = LoadedVoiceSet;
		LoadingVoicePath.Reset();
		return;
	}

	LoadingVoicePath = NewVoiceSet.ToSoftObjectPath();
	VoiceLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(LoadingVoicePath,
		FStreamableDelegate::CreateUObject(this, &ThisClass::OnVoiceLoaded));
}

void UHeirAppearanceComponent::OnVoiceLoaded()
{
	VoiceSet = Cast<UGodsentVoiceSetDataAsset>(LoadingVoicePath.ResolveObject());
	LoadingVoicePath.Reset();

	if (bPlayRandomLineWhenLoaded)
	{
		bPlayRandomLineWhenLoaded = false;
		PlayRandomVoiceLine();
	}
}

UAudioComponent* UHeirAppearanceComponent::PlaySound(USoundBase* Sound)
{
	if (!Sound)
		return nullptr;

	if (bInterruptPreviousVoiceLine)
		StopVoiceLine();

	USceneComponent* AttachTo = GetTargetMesh();

	if (!AttachTo && GetOwner())
		AttachTo = GetOwner()->GetRootComponent();

	if (!AttachTo)
		return nullptr;

	UAudioComponent* Line = UGameplayStatics::SpawnSoundAttached(Sound, AttachTo);
	ActiveVoiceLine = Line;
	return Line;
}
