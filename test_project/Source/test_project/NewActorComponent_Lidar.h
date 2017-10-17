#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NewActorComponent_Lidar.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TEST_PROJECT_API UNewActorComponent_Lidar : public UActorComponent
{
    GENERATED_BODY()

public:	
        // Sets default values for this component's properties
	UNewActorComponent_Lidar();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
        // Called every frame
        virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
        int ChangeFloorTexture(FString* f);
        void timer_expire();
        void ZoomIn();
        void GetLidarScan();
        void GetScreenshot();
};
