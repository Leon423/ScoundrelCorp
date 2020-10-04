// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SCharacter.generated.h"

//forward declare classes
class UCameraComponent;
class USpringArmComponent;
class UInputComponent;
class ASWeapon;
class USHealthComponent;

UCLASS()
class SCOUNDRELCORP_API ASCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float value);

	void MoveRight(float value);

	void BeginCrouch();

	void EndCrouch();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;

	bool bWantsToZoom;	

	/* Default FOV set during begin play*/
	float DefaultFOV;

	void Zoom();

	void Unzoom();

	UPROPERTY(Replicated)
	ASWeapon* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Player")
	TSubclassOf<ASWeapon> StarterWeaponClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponAttachSocketName;

	

	UFUNCTION()
    void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	//pawn died previously
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Player")
	bool bDied;

	void HandleZoom(float DeltaTime);

	// abilities
	UPROPERTY(EditDefaultsOnly, Category = "Player")
	float AbilityCooldown;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_Ability)
	uint32 bPendingAbility : 1;

	UFUNCTION()
    void OnRep_Ability();

	UFUNCTION(BlueprintImplementableEvent, Category = "Player/Ability")
		void PerformAbility();
	
	FTimerHandle TimerHandle_AbilityTime;
	
	float lastAbilityTime;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;

	// start and stop fire must be public so we can call it from Behavior Trees for the AI to use them.
	UFUNCTION(BlueprintCallable, Category = "Player")
        void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Player")
        void StopFire();

	UFUNCTION(BlueprintCallable, Category = "Player")
		void StartReload();
	UFUNCTION(BlueprintCallable, Category = "Player")
		void StopReload();

	UFUNCTION(BlueprintCallable, Category = "Player")
		void StartAbility();
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerStartAbility();

	void CompleteAbility();

	
};
