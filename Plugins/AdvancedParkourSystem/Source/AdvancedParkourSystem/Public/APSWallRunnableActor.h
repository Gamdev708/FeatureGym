// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "APSWallRunnableInterface.h"
#include "GameFramework/Actor.h"
#include "APSWallRunnableActor.generated.h"

UCLASS()
class ADVANCEDPARKOURSYSTEM_API AAPSWallRunnableActor : public AActor,public IAPSWallRunnableInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAPSWallRunnableActor();
};
