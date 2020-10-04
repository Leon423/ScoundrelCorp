// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Particles/ParticleSystem.h"
#include "SWeapon.generated.h"

class USkeletalMeshComponent;
class UDamageType;
class UParticleSystem;
class UCameraShake;

// Contains information of a single hitscan weapon linetrace
USTRUCT()
struct FHitScanTrace
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class SCOUNDRELCORP_API ASWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASWeapon();
	void PostInitializeComponents();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* MuzzleEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName;

	void PlayFireEffects(FVector TraceEnd);

	void PlayReloadEffects();

	void PlayImpactEffects(EPhysicalSurface SurfaceType, FVector ImpactPoint);

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCamShake;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float HeadshotDamageMultiplier;
	
	FTimerHandle TimerHandle_TimeBetweenShots;

	float LastFireTime;

	/*RPM - Bullets per minute fired by weapon.*/
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;

	/*Derived from rate of fire*/
	float TimeBetweenShots;

	UPROPERTY(Transient, Replicated)
	uint32 bIsFiring : 1;

	/*Bullet spread in degrees*/
	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0f))
	float BulletSpread;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

	UFUNCTION()
    void OnRep_HitScanTrace();

	bool CanFire() const;

	//Weapons have their own zoom and zoom speed
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float ZoomedFOV;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon", meta = (ClampMin = 0.0, ClampMax = 100))
	float ZoomInterpSpeed;

	// Ammo stuff
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon/Ammo")
	int32 MaxAmmo;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon/Ammo")
	int32 AmmoPerMag;

	UPROPERTY(VisibleAnywhere, Category = "Weapon/Ammo")
	int32 InitialMags;

	/** Current Total Ammo */
	UPROPERTY(VisibleAnywhere,Transient, Replicated)
	int32 CurrentAmmo;
	/**Current ammo in magazine*/
	UPROPERTY(VisibleAnywhere,Transient, Replicated)
	int32 CurrentAmmoInMag;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon/Ammo")
	float ReloadTime;
	
	FTimerHandle TimerHandle_ReloadTime;

	bool CanReload() const;

	UFUNCTION()
    void OnRep_Reload();
	
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Reload)
	uint32 bPendingReload : 1;
	
public:	

	//called on server and local client. HitScanTrace used to replicate shot effects to other clients.
	virtual void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
    void ServerFire();

	void StartFire();

	void StopFire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerReload();

	//called on server and local client. Reload logic is done here.
	virtual void Reload();

	void CompleteReload();
	
	void StartReload();
	void StopReload();

	float GetZoomedFOV(){return ZoomedFOV;}
	float GetZoomSpeed(){return ZoomInterpSpeed;}
};
