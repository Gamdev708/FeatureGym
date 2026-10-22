// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FeatureGym2GameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AFeatureGym2GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AFeatureGym2GameMode();
};



