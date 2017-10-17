#include "NewActorComponent_Lidar.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/GameFramework/Character.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include <string>

UActorComponent* Lidar = NULL;
UCameraComponent* L = NULL;
AActor* floor_mesh = NULL;
FString test_string = "";
FString floor_material_string = "";
int test_size = -1;
UMaterial* NFMaterial = NULL;
FTimerHandle TestTimerHandle;

void UNewActorComponent_Lidar::timer_expire() {
    GetWorld()->GetTimerManager().SetTimer(TestTimerHandle, this, &UNewActorComponent_Lidar::timer_expire, 1.0f, true);
}

void UNewActorComponent_Lidar::ZoomIn()
{
    //bZoomingIn = true;
}

// Sets default values for this component's properties
UNewActorComponent_Lidar::UNewActorComponent_Lidar()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    // ...
    //get a pointer to a new floor material
    static ConstructorHelpers::FObjectFinder<UMaterial> New_Floor_Material(TEXT("Material'/Game/StarterContent/Materials/M_Tech_Hex_Tile_Pulse.M_Tech_Hex_Tile_Pulse'"));
    if (New_Floor_Material.Succeeded()) {
        NFMaterial = (UMaterial*)New_Floor_Material.Object;
    }
}

// Called when the game starts
void UNewActorComponent_Lidar::BeginPlay()
{
    Super::BeginPlay();
    Lidar = (GetOwner()->GetComponentsByClass(UCameraComponent::StaticClass()))[1];
    L = Cast<UCameraComponent> (Lidar);

    //change floor texture
    //test_size = ChangeFloorTexture(&test_string);

    // ...
    //setup the input component to capture a screenshot on a keypress
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    OwnerCharacter->InputComponent->BindAction("ScreenShotMode", IE_Pressed, this, &UNewActorComponent_Lidar::GetScreenshot);
}

int UNewActorComponent_Lidar::ChangeFloorTexture(FString* f)
{
    UWorld* World = GetWorld();
    TArray<AActor*> FoundActors;
    FString floor_string = "Floor";

    //get all the static mesh actors
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FoundActors);

    for (AActor* TActor: FoundActors)
    {
        if(TActor != nullptr)
            if ((TActor->GetName()).Compare(floor_string) == 0) {
                //found the floor mesh
                floor_mesh = TActor;
                test_string = TActor->GetName();

                //get the components of the floor
                TArray <UStaticMeshComponent *> Floor_Components;
                TActor->GetComponents<UStaticMeshComponent>(Floor_Components);

                //UMaterialInstanceDynamic* DMaterial = UMaterialInstanceDynamic::Create(Floor_Components[0]->GetMaterial(0), nullptr);
                //floor_material_string = *DMaterial->GetName(); 

                if (NFMaterial != NULL) {
                    UMaterialInstanceDynamic* NFMaterial_Instance = UMaterialInstanceDynamic::Create(NFMaterial, Floor_Components[0]);
                    //set the new material to the floor
                    Floor_Components[0]->SetMaterial(0,NFMaterial_Instance);
                }
                return Floor_Components.Num();
            }
    }

    return FoundActors.Num();
}

void UNewActorComponent_Lidar::GetLidarScan() {
    float x=0;
    FString UE4Str;
    FVector current_location, end_location;
    FRotator current_rotation;
    FVector current_forward_vector;
    
    if (Lidar == NULL) {
        Lidar = (GetOwner()->GetComponentsByClass(UCameraComponent::StaticClass()))[1]; //Lidar needs to be the second camera in the component list
        L = Cast<UCameraComponent> (Lidar);
    }
    
    UE4Str = Lidar->GetName();
    x = L->GetComponentLocation().X;
    current_location = L->GetComponentLocation();
    current_rotation = L->GetComponentRotation();
    current_forward_vector = L->GetForwardVector();

    //Set collision query parameters
    FCollisionQueryParams TraceParams;
    TraceParams.bFindInitialOverlaps = true; // Doesn't change anything whether it's true by default or false in my case
    TraceParams.AddIgnoredActor(GetOwner());
    TraceParams.bTraceComplex = true;
    TraceParams.bTraceAsyncScene = true;
    TraceParams.bReturnPhysicalMaterial = false;

    for(int i=-30;i<30;i++) {
        FRotator r = FRotator(0,i*1.5,0);
        end_location = current_location + (r.RotateVector(current_forward_vector) * 2500);

        //Re-initialize hit info
        FHitResult RV_Hit(ForceInit);

        //Trace out each line 
        if (GetWorld()->LineTraceSingleByChannel(RV_Hit,current_location, end_location, ECC_Pawn, TraceParams) == true) {
            //there was an impact, then draw the trace in red
            FVector impact = RV_Hit.Location;

            DrawDebugLine(GetWorld(), current_location, impact, FColor(255,0,0), false, -1, 0, .33);
            DrawDebugPoint(GetWorld(), impact, 20, FColor(255,0,255), false, 0.03);
            GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Red, FString::Printf(TEXT("Lidar distance ~> %s %s %d"), *test_string, *floor_material_string, test_size));
        } else {
            //there was no impact, so draw the trace in green
            DrawDebugLine(GetWorld(), current_location, end_location, FColor(0,255,0), false, -1, 0, .33);
        }

    }
}

void UNewActorComponent_Lidar::GetScreenshot() {
    GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("Requesting screenshot")); 
    FString fileName("/Users/jseng/screenshot.png");
    FScreenshotRequest::RequestScreenshot(fileName, false, false);
}

// Called every frame
void UNewActorComponent_Lidar::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    GetLidarScan();
}
