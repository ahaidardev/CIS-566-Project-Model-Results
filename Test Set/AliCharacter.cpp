// Fill out your copyright notice in the Description page of Project Settings.

//#include "Public/AliCharacter.h"
// #include "AlsCameraComponent.h"
//#include "WOTDRefactored/Player/Public/AliCharacter.h"
#include "WOTDRefactored/Public/Player/AliCharacter.h"

#include "AlsCameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ALS/Public/Utility/AlsVector.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GroomComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameNetworkManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Settings/AlsCharacterSettings.h"
// #include "Player/InventoryComponent.h"
#include "Player/AliCharacter.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Components/InventoryComponent.h"
#include "Player/Components/ParkourComponent.h"
#include "Player/Components/CoverComponent.h"
#include "Player/Components/AimObstructionComponent.h"
#include "Player/Components/PlayerSFXComponent.h"
#include "Abilities/GSCAbilitySystemComponent.h"
#include "ContextualAnimSceneActorComponent.h"
#include "Misc/DataValidation.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StealthSystem/StealthGameplayTags.h"

// void AAliCharacter::NormalWeaponShot()
// {
// }

class UProjectileMovementComponent;

#include "AliCharacter/01_VisibilityAndConstruction.inl"

#include "AliCharacter/02_Invisibility.inl"

#include "AliCharacter/03_LifecycleAndCamera.inl"

#include "AliCharacter/04_InputBindings.inl"

#include "AliCharacter/05_LocomotionInput.inl"

#include "AliCharacter/06_CoverAndWeaponInput.inl"

#include "AliCharacter/07_Initialization.inl"

#include "AliCharacter/08_WeaponTracing.inl"

#include "AliCharacter/09_InteractionTracing.inl"

#include "AliCharacter/10_CameraAndObstacleChecks.inl"

#include "AliCharacter/11_PowerWeaponAndViewMode.inl"
