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
	template <typename TPerceptionCallback>
	static void ReplayCurrentlyPerceivedActors(UAIPerceptionComponent* TrackerPerception, ATrackerEnemy* Tracker,
		TPerceptionCallback OnTrackerPerceptionUpdated)
	{
		TArray<AActor*> SeenActors;
		TrackerPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), SeenActors);
		for (AActor* Actor : SeenActors)
		{
			OnTrackerPerceptionUpdated(
				Actor, FAIStimulus(*GetDefault<UAISense_Sight>(), 1.0f, Actor->GetActorLocation(), Tracker->GetActorLocation()));
		}
	}

	template <typename TPlayerCollection>
	static bool HasVisiblePlayer(const TPlayerCollection& VisiblePlayers)
	{
		for (const TWeakObjectPtr<AActor>& Player : VisiblePlayers)
		{
			if (Player.IsValid())
				return true;
		}
		return false;
	}

	template <typename TVisiblePlayerCollection, typename TPingedPlayerCollection, typename TSightLeadCallback>
	static void HandlePlayerSightStimulus(AActor* Actor, const FAIStimulus& Stimulus,
		TVisiblePlayerCollection& VisiblePlayers, TPingedPlayerCollection& PingedVisiblePlayers,
		TSightLeadCallback TryPlayerSightLead)
	{
		APawn* Player = Cast<APawn>(Actor);
		if (!Player || !Player->GetController() || !Player->GetController()->IsPlayerController())
			return;
		if (!Stimulus.WasSuccessfullySensed())
		{
			VisiblePlayers.Remove(Actor);
			PingedVisiblePlayers.Remove(Actor);
			return;
		}
		VisiblePlayers.Add(Actor);
		TryPlayerSightLead(Actor, Stimulus.StimulusLocation);
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
	TrackerPerception.Reset();
	TrackerPawnSensing.Reset();
	VisiblePlayers.Reset();
	PingedVisiblePlayers.Reset();
	TrackerSuspicion.Reset();
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
	TrackerPerception = GetController()->FindComponentByClass<UAIPerceptionComponent>();
	TrackerPawnSensing = GetController()->FindComponentByClass<UPawnSensingComponent>();
	if (TrackerPerception.IsValid())
	{
		TrackerPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ATrackerEnemy::OnTrackerPerceptionUpdated);
		ReplayCurrentlyPerceivedActors(TrackerPerception.Get(), this, [this](AActor* Actor, FAIStimulus Stimulus)
		{
			OnTrackerPerceptionUpdated(Actor, Stimulus);
		});
	}
	if (TrackerPawnSensing.IsValid())
		TrackerPawnSensing->OnHearNoise.AddUniqueDynamic(this, &ATrackerEnemy::OnTrackerPawnNoise);
}

void ATrackerEnemy::OnTrackerPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		HandlePlayerSightStimulus(Actor, Stimulus, VisiblePlayers, PingedVisiblePlayers,
			[this](AActor* Player, const FVector& Location)
		{
			TryPlayerSightLead(Player, Location);
		});
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Stimulus.WasSuccessfullySensed())
	{
		// Squad identification shouts are not new investigation leads.
		if (Stimulus.Tag != TEXT("TargetIdentifiedEnemyShout"))
			HandleNoiseLead(Actor, Stimulus.StimulusLocation);
	}
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
	if (!GetWorld() || Source == this || (Source && Source->IsA<AABaseEnemy>()))
		return;
	if (HasVisiblePlayer(VisiblePlayers))
		return;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastNoisePingTime < FMath::Max(0.0f, NoisePingCooldown))
		return;
	if (PingTarget(Location))
		LastNoisePingTime = Now;
}

void ATrackerEnemy::OnTrackerPlayerIdentified(AActor* Player)
{
	const APawn* PlayerPawn = Cast<APawn>(Player);
	if (!PlayerPawn || !PlayerPawn->GetController() || !PlayerPawn->GetController()->IsPlayerController() ||
		!GetWorld())
		return;
	USquadCoordinatorSubsystem* Coordinator = GetWorld()->GetSubsystem<USquadCoordinatorSubsystem>();
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
	USquadCoordinatorSubsystem* Coordinator =
		GetWorld() ? GetWorld()->GetSubsystem<USquadCoordinatorSubsystem>() : nullptr;
	return Coordinator && Coordinator->PingTrackerSquad(this, SearchLocation);
}

void ATrackerEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindTrackerPerception();
	if (GetWorld())
	{
		if (USquadCoordinatorSubsystem* Coordinator = GetWorld()->GetSubsystem<USquadCoordinatorSubsystem>())
		{
			Coordinator->ReleaseTracker(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}
