#include "Enemy/TrackerEnemy.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "StealthSystem/Subsystems/SquadCoordinatorSubsystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/PawnSensingComponent.h"

namespace
{
	static TArray<AActor*> GetCurrentlySeenActors(UAIPerceptionComponent* Perception)
	{
		TArray<AActor*> SeenActors;
		Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), SeenActors);
		return SeenActors;
	}

	static FAIStimulus MakeSightStimulus(const AActor* Actor, const FVector& ReceiverLocation)
	{
		return FAIStimulus(*GetDefault<UAISense_Sight>(), 1.0f, Actor->GetActorLocation(), ReceiverLocation);
	}

	static bool IsPlayerControlledPawn(const APawn* Pawn)
	{
		return Pawn && Pawn->GetController() && Pawn->GetController()->IsPlayerController();
	}

	template <typename ContainerType>
	static bool HasAnyValidPlayer(const ContainerType& Players)
	{
		for (const TWeakObjectPtr<AActor>& Player : Players)
		{
			if (Player.IsValid())
				return true;
		}
		return false;
	}

	static USquadCoordinatorSubsystem* GetSquadCoordinator(const UWorld* World)
	{
		return World ? World->GetSubsystem<USquadCoordinatorSubsystem>() : nullptr;
	}

	static void ClearTrackerPerceptionCaches(
		TWeakObjectPtr<UAIPerceptionComponent>& Perception,
		TWeakObjectPtr<UPawnSensingComponent>& PawnSensing,
		TArray<TWeakObjectPtr<AActor>>& VisiblePlayers,
		TArray<TWeakObjectPtr<AActor>>& PingedVisiblePlayers,
		TWeakObjectPtr<UModularSuspicionComponent>& Suspicion)
	{
		Perception.Reset();
		PawnSensing.Reset();
		VisiblePlayers.Reset();
		PingedVisiblePlayers.Reset();
		Suspicion.Reset();
	}

	struct FControllerSenseComponents
	{
		UAIPerceptionComponent* Perception = nullptr;
		UPawnSensingComponent* PawnSensing = nullptr;
	};

	static FControllerSenseComponents FindControllerSenseComponents(AController* Controller)
	{
		FControllerSenseComponents Components;
		Components.Perception = Controller->FindComponentByClass<UAIPerceptionComponent>();
		Components.PawnSensing = Controller->FindComponentByClass<UPawnSensingComponent>();
		return Components;
	}

	static bool TrackSightedPlayer(
		AActor* Actor,
		bool bWasSuccessfullySensed,
		TArray<TWeakObjectPtr<AActor>>& VisiblePlayers,
		TArray<TWeakObjectPtr<AActor>>& PingedVisiblePlayers)
	{
		APawn* Player = Cast<APawn>(Actor);
		if (!IsPlayerControlledPawn(Player))
			return false;
		if (!bWasSuccessfullySensed)
		{
			VisiblePlayers.Remove(Actor);
			PingedVisiblePlayers.Remove(Actor);
			return false;
		}
		VisiblePlayers.Add(Actor);
		return true;
	}

	static bool IsInvestigationHearingStimulus(const FAIStimulus& Stimulus)
	{
		// Squad identification shouts are not new investigation leads.
		return Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>()
			&& Stimulus.WasSuccessfullySensed()
			&& Stimulus.Tag != TEXT("TargetIdentifiedEnemyShout");
	}

	static bool IsNoiseLeadSuppressed(
		const UWorld* World,
		const AActor* Self,
		const AActor* Source,
		const TArray<TWeakObjectPtr<AActor>>& VisiblePlayers,
		double LastNoisePingTime,
		float NoisePingCooldown,
		double& OutNow)
	{
		if (!World || Source == Self || (Source && Source->IsA<AABaseEnemy>()))
			return true;
		if (HasAnyValidPlayer(VisiblePlayers))
			return true;
		OutNow = World->GetTimeSeconds();
		return OutNow - LastNoisePingTime < FMath::Max(0.0f, NoisePingCooldown);
	}

	static USquadCoordinatorSubsystem* GetIdentificationCoordinator(const AActor* Player, const UWorld* World)
	{
		const APawn* PlayerPawn = Cast<APawn>(Player);
		if (!IsPlayerControlledPawn(PlayerPawn))
			return nullptr;
		return GetSquadCoordinator(World);
	}

	static void ReleaseTrackerFromSquad(ATrackerEnemy* Enemy, const UWorld* World)
	{
		if (USquadCoordinatorSubsystem* Coordinator = GetSquadCoordinator(World))
			Coordinator->ReleaseTracker(Enemy);
	}
}

void ATrackerEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	BindTrackerPerception();
}

void ATrackerEnemy::UnPossessed()
{
	UnbindTrackerPerception();
	Super::UnPossessed();
}

void ATrackerEnemy::BeginPlay()
{
	Super::BeginPlay();
	BindTrackerPerception();
}

void ATrackerEnemy::UnbindTrackerPerception()
{
	if (TrackerPerception.IsValid())
		TrackerPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ATrackerEnemy::OnTrackerPerceptionUpdated);
	if (TrackerPawnSensing.IsValid())
		TrackerPawnSensing->OnHearNoise.RemoveDynamic(this, &ATrackerEnemy::OnTrackerPawnNoise);
	if (TrackerSuspicion.IsValid())
		TrackerSuspicion->OnSuspicionColorChanged.RemoveDynamic(this, &ATrackerEnemy::OnTrackerSuspicionChanged);
	if (TrackerSuspicion.IsValid())
		TrackerSuspicion->OnTargetIdentifiedEvent.RemoveDynamic(this, &ATrackerEnemy::OnTrackerPlayerIdentified);
	ClearTrackerPerceptionCaches(TrackerPerception, TrackerPawnSensing, VisiblePlayers, PingedVisiblePlayers, TrackerSuspicion);
}

void ATrackerEnemy::BindTrackerPerception()
{
	UnbindTrackerPerception();
	if (!GetController())
		return;
	TrackerSuspicion = FindComponentByClass<UModularSuspicionComponent>();
	if (TrackerSuspicion.IsValid())
		TrackerSuspicion->OnSuspicionColorChanged.AddUniqueDynamic(this, &ATrackerEnemy::OnTrackerSuspicionChanged);
	if (TrackerSuspicion.IsValid())
		TrackerSuspicion->OnTargetIdentifiedEvent.AddUniqueDynamic(this, &ATrackerEnemy::OnTrackerPlayerIdentified);
	const FControllerSenseComponents SenseComponents = FindControllerSenseComponents(GetController());
	TrackerPerception = SenseComponents.Perception;
	TrackerPawnSensing = SenseComponents.PawnSensing;
	if (TrackerPerception.IsValid())
	{
		TrackerPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ATrackerEnemy::OnTrackerPerceptionUpdated);
		for (AActor* Actor : GetCurrentlySeenActors(TrackerPerception.Get()))
		{
			OnTrackerPerceptionUpdated(Actor, MakeSightStimulus(Actor, GetActorLocation()));
		}
	}
	if (TrackerPawnSensing.IsValid())
		TrackerPawnSensing->OnHearNoise.AddUniqueDynamic(this, &ATrackerEnemy::OnTrackerPawnNoise);
}

void ATrackerEnemy::OnTrackerPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		if (TrackSightedPlayer(Actor, Stimulus.WasSuccessfullySensed(), VisiblePlayers, PingedVisiblePlayers))
			TryPlayerSightLead(Actor, Stimulus.StimulusLocation);
		return;
	}
	if (IsInvestigationHearingStimulus(Stimulus))
		HandleNoiseLead(Actor, Stimulus.StimulusLocation);
}

bool ATrackerEnemy::IsSuspicious() const
{
	return TrackerSuspicion.IsValid() && TrackerSuspicion->GetColorStatus() != ESuspicionColorStatus::Blue;
}

void ATrackerEnemy::TryPlayerSightLead(AActor* Player, const FVector& Location)
{
	if (IsSuspicious() && !PingedVisiblePlayers.Contains(Player) && PingTarget(Location))
	{
		PingedVisiblePlayers.Add(Player);
	}
}

void ATrackerEnemy::OnTrackerSuspicionChanged(ESuspicionColorStatus Color)
{
	if (Color == ESuspicionColorStatus::Blue)
		return;
	for (const TWeakObjectPtr<AActor>& Player : VisiblePlayers)
	{
		if (Player.IsValid())
			TryPlayerSightLead(Player.Get(), Player->GetActorLocation());
	}
}

void ATrackerEnemy::HandleNoiseLead(AActor* Source, const FVector& Location)
{
	double Now = 0.0;
	if (IsNoiseLeadSuppressed(GetWorld(), this, Source, VisiblePlayers, LastNoisePingTime, NoisePingCooldown, Now))
		return;
	if (PingTarget(Location))
		LastNoisePingTime = Now;
}

void ATrackerEnemy::OnTrackerPlayerIdentified(AActor* Player)
{
	USquadCoordinatorSubsystem* Coordinator = GetIdentificationCoordinator(Player, GetWorld());
	if (!Coordinator)
		return;
	if (!Coordinator->ShareTrackerIdentification(this, Player) && PingTarget(Player->GetActorLocation()))
	{
		Coordinator->ShareTrackerIdentification(this, Player);
	}
}

void ATrackerEnemy::OnTrackerPawnNoise(APawn* Pawn, const FVector& Location, float Volume)
{
	if (Volume > 0.0f)
		HandleNoiseLead(Pawn, Location);
}

bool ATrackerEnemy::PingTarget(FVector SearchLocation)
{
	USquadCoordinatorSubsystem* Coordinator = GetSquadCoordinator(GetWorld());
	return Coordinator && Coordinator->PingTrackerSquad(this, SearchLocation);
}

void ATrackerEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindTrackerPerception();
	ReleaseTrackerFromSquad(this, GetWorld());
	Super::EndPlay(EndPlayReason);
}
