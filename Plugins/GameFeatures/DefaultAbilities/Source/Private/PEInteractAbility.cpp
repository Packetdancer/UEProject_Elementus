// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEProject_Elementus

#include "PEInteractAbility.h"
#include "Actors/Character/PECharacter.h"
#include "Actors/Interfaces/PEInteractable.h"
#include "GAS/System/PEAbilitySystemComponent.h"
#include "Tasks/PEInteractAbility_Task.h"

UPEInteractAbility::UPEInteractAbility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("GameplayAbility.Default.Interact"));
	bAutoActivateOnGrant = true;
	bWaitCancel = false;

	AbilityMaxRange = 1000.f;
}

void UPEInteractAbility::ActivateAbility
(const FGameplayAbilitySpecHandle Handle,
 const FGameplayAbilityActorInfo* ActorInfo,
 const FGameplayAbilityActivationInfo ActivationInfo,
 const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	TaskHandle = UPEInteractAbility_Task::InteractionTask(this, "InteractTask", AbilityMaxRange);
	TaskHandle->ReadyForActivation();
}

void UPEInteractAbility::InputPressed(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (TaskHandle.IsValid() && TaskHandle->GetIsInteractAllowed())
	{
		if (IsValid(TaskHandle->GetInteractable()) &&
			IPEInteractable::Execute_IsInteractEnabled(TaskHandle->GetInteractable()))
		{
			IPEInteractable::Execute_DoInteractionBehavior(TaskHandle->GetInteractable(), ActorInfo->AvatarActor.Get());
		}
	}
}
