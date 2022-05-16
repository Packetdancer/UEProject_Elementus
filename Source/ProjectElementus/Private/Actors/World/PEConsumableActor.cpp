// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEProject_Elementus

#include "Actors/World/PEConsumableActor.h"

#include "AbilitySystemGlobals.h"
#include "GAS/System/PEAbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Actors/Character/PECharacter.h"

APEConsumableActor::APEConsumableActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  bDestroyAfterConsumption(true)
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	ObjectMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Object Mesh"));
	ObjectMesh->SetCollisionProfileName("Consumable");
	ObjectMesh->SetupAttachment(RootComponent);

	ObjectVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Object VFX"));
	ObjectVFX->bAutoActivate = true;
	ObjectVFX->SetupAttachment(ObjectMesh);
}

void APEConsumableActor::PerformConsumption(UAbilitySystemComponent* TargetABSC)
{
	if (UPEAbilitySystemComponent* TargetGASC = Cast<UPEAbilitySystemComponent>(TargetABSC);
		ensureMsgf(IsValid(TargetGASC), TEXT("%s have a invalid target"), *GetName()))
	{
		if (GetLocalRole() != ROLE_Authority)
		{
			if (bDestroyAfterConsumption)
			{
				Destroy();
			}

			return;
		}

		if (TargetGASC->HasAllMatchingGameplayTags(ConsumableData->RequirementsTags)
			|| ConsumableData->RequirementsTags.IsEmpty())
		{
			for (const FGameplayEffectGroupedData& Effect : ConsumableData->ConsumableEffects)
			{
				TargetGASC->ApplyEffectGroupedDataToSelf(Effect);
			}

			if (bDestroyAfterConsumption)
			{
				Destroy();
			}
		}
	}
}

void APEConsumableActor::DoInteractionBehavior_Implementation(APECharacter* CharacterInteracting,
                                                              const FHitResult& HitResult)
{
	if (CharacterInteracting->Implements<UAbilitySystemInterface>())
	{
		if (UPEAbilitySystemComponent* AbilitySystemComponent =
				Cast<UPEAbilitySystemComponent>(CharacterInteracting->GetAbilitySystemComponent());
			ensureAlwaysMsgf(IsValid(AbilitySystemComponent), TEXT("%s have a invalid AbilitySystemComponent"),
			                 *GetName()))
		{
			SetReplicates(true);
			PerformConsumption(AbilitySystemComponent);
			SetReplicates(false);
		}
	}
}

#if WITH_EDITOR
void APEConsumableActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APEConsumableActor, ConsumableData))
		{
			if (IsValid(ConsumableData))
			{
				!ConsumableData->ObjectMesh.IsNull()
					? ObjectMesh->SetStaticMesh(ConsumableData->ObjectMesh.LoadSynchronous())
					: ObjectMesh->SetStaticMesh(nullptr);

				!ConsumableData->ObjectVFX.IsNull()
					? ObjectVFX->SetAsset(ConsumableData->ObjectVFX.LoadSynchronous())
					: ObjectVFX->SetAsset(nullptr);
			}
		}
	}
}
#endif WITH_EDITOR
