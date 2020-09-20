// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameMode.h"
#include "ScoundrelCorp/Components/SHealthComponent.h"

ASGameMode::ASGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.0f;
}

void ASGameMode::StartPlay()
{
    Super::StartPlay();
}

void ASGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

