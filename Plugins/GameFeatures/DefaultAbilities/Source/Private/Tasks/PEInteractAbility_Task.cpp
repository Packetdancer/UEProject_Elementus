// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEProject_Elementus

#include "Tasks/PEInteractAbility_Task.h"
#include <Interfaces/PEInteractable.h>
#include <Targeting/PELineTargeting.h>
#include <Abilities/GameplayAbilityTargetDataFilter.h>
#include <Abilities/Tasks/AbilityTask_WaitGameplayTag.h>
#include <Camera/CameraComponent.h>
#include <AbilitySystemComponent.h>
#include <PEAbilityTags.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(PEInteractAbility_Task)

UPEInteractAbility_Task::UPEInteractAbility_Task(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bTickingTask = false;
	bIsFinished = false;
}

UPEInteractAbility_Task* UPEInteractAbility_Task::InteractionTask(UGameplayAbility* OwningAbility, const FName& TaskInstanceName, const float InteractionRange, const bool bUseCustomDepth)
{
	UPEInteractAbility_Task* const MyObj = NewAbilityTask < UPEInteractAbility_Task > (OwningAbility, TaskInstanceName);
	MyObj->Range = InteractionRange;
	MyObj->bUseCustomDepth = bUseCustomDepth;

	return MyObj;
}

void UPEInteractAbility_Task::Activate()
{
	Super::Activate();
	check(Ability);

	if (UActorComponent* const CameraComp = GetAvatarActor()->GetComponentByClass(UCameraComponent::StaticClass()); IsValid(CameraComp))
	{
		OwningCameraComponent = Cast<UCameraComponent>(CameraComp);

		if (!Ability->GetActorInfo().IsNetAuthority())
		{
			return;
		}

		UAbilityTask_WaitGameplayTagAdded* const WaitGameplayTagAdd = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(Ability, FGameplayTag::RequestGameplayTag(TEXT("State.CannotInteract")));
		WaitGameplayTagAdd->Added.AddDynamic(this, &UPEInteractAbility_Task::OnCannotInteractChanged);

		UAbilityTask_WaitGameplayTagRemoved* const WaitGameplayTagRemove = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(Ability, FGameplayTag::RequestGameplayTag(TEXT("State.CannotInteract")));
		WaitGameplayTagRemove->Removed.AddDynamic(this, &UPEInteractAbility_Task::OnCannotInteractChanged);

		WaitGameplayTagAdd->ReadyForActivation();
		WaitGameplayTagRemove->ReadyForActivation();

		bTickingTask = true;
		return;
	}

	bIsFinished = true;
	EndTask();
}

bool UPEInteractAbility_Task::GetIsInteractAllowed() const
{
	return AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(GlobalTag_CanInteract)) && !AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(GlobalTag_CannotInteract));
}

AActor* UPEInteractAbility_Task::GetInteractable() const
{
	return LastInteractableActor_Ref.Get();
}

FHitResult UPEInteractAbility_Task::GetInteractableHitResult() const
{
	return HitResult;
}

void UPEInteractAbility_Task::UpdateInteractableTarget()
{
	HitResult.Reset(0.f, false);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Ability->GetAvatarActorFromActorInfo());

	const FVector StartLocation = OwningCameraComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation + OwningCameraComponent->GetForwardVector() * Range;

	const FGameplayTargetDataFilterHandle DataFilterHandle;

	APELineTargeting::LineTraceWithFilter(HitResult, GetWorld(), DataFilterHandle, StartLocation, EndLocation, TEXT("Target"), QueryParams);

	if (!HitResult.bBlockingHit || !IsValid(HitResult.GetActor()) || !HitResult.GetActor()->Implements<UPEInteractable>())
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(GlobalTag_CanInteract)))
		{
			AbilitySystemComponent->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(GlobalTag_CanInteract));
		}

		if (LastInteractableActor_Ref.IsValid())
		{
			IPEInteractable::Execute_SetIsCurrentlyFocusedByActor(LastInteractableActor_Ref.Get(), false, OwningCameraComponent->GetOwner(), HitResult);
			LastInteractableActor_Ref.Reset();

			if (LastInteractablePrimitive_Ref.IsValid())
			{
				if (bUseCustomDepth)
				{
					LastInteractablePrimitive_Ref->SetRenderCustomDepth(false);
				}

				LastInteractablePrimitive_Ref.Reset();
			}
		}

		return;
	}

	if (LastInteractableActor_Ref.Get() != HitResult.GetActor())
	{
		LastInteractableActor_Ref = HitResult.GetActor();
		LastInteractablePrimitive_Ref = HitResult.GetComponent();

		IPEInteractable::Execute_SetIsCurrentlyFocusedByActor(LastInteractableActor_Ref.Get(), true, OwningCameraComponent->GetOwner(), HitResult);

		if (bUseCustomDepth)
		{
			LastInteractablePrimitive_Ref->SetRenderCustomDepth(true);
		}

		AbilitySystemComponent->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(GlobalTag_CanInteract));
	}
}

void UPEInteractAbility_Task::OnCannotInteractChanged()
{
	bTickingTask = !AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(GlobalTag_CannotInteract));
}

void UPEInteractAbility_Task::TickTask(const float DeltaTime)
{
	if (bIsFinished)
	{
		EndTask();
		return;
	}

	Super::TickTask(DeltaTime);

	UpdateInteractableTarget();
}

void UPEInteractAbility_Task::OnDestroy(const bool AbilityIsEnding)
{
	bIsFinished = true;

	if (LastInteractableActor_Ref.IsValid())
	{
		IPEInteractable::Execute_SetIsCurrentlyFocusedByActor(LastInteractableActor_Ref.Get(), false, OwningCameraComponent->GetOwner(), HitResult);
		if (LastInteractablePrimitive_Ref.IsValid())
		{
			LastInteractablePrimitive_Ref->SetRenderCustomDepth(false);
		}
	}

	Super::OnDestroy(AbilityIsEnding);
}
