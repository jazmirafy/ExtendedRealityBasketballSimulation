// fill out your copyright notice in the description page of project settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class UCameraComponent;

// interactable physics object that can be held in front of the player or thrown on release
UCLASS()
class XRBASKETBALLSIM_API APickup : public AActor
{
	GENERATED_BODY()

public:
	// sets default values for pickup state and mesh physics
	APickup();

protected:
	// caches player references and finds the holding component on begin play
	virtual void BeginPlay() override;

public:
	// when held, follows the holding component transform every frame
	virtual void Tick(float DeltaTime) override;

	// mesh used for visuals + physics simulation
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* MyMesh;

	// scene component on the player that defines where held objects should sit
	UPROPERTY(EditAnywhere)
	USceneComponent* HoldingComp;

	// impulse strength applied when the item is released
	UPROPERTY(EditAnywhere)
	float ForceAmount = 1000.f;

	// rotates the pickup to match the player's control rotation (useful for inspect mode)
	UFUNCTION()
	void RotateActor();

	// toggles between held and released states (disables physics when held, applies impulse when released)
	UFUNCTION()
	void Pickup();

	// true when the object is currently attached to the holding component
	bool bHolding;

	// gravity toggle used when switching between held and released states
	bool bGravity;

	// cached rotation used for aligning the object during inspection
	FRotator ControlRotation;

	// cached player character reference (used to find camera + holding component)
	ACharacter* MyCharacter;

	// cached camera component used to throw in the camera forward direction
	UCameraComponent* PlayerCamera;

	// cached forward direction used for impulse throw
	FVector ForwardVector;
};

