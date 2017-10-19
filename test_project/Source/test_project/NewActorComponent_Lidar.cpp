#include "NewActorComponent_Lidar.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/GameFramework/Character.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/Engine/Brush.h"
#include <string>

UCameraComponent* L = NULL;
AActor* floor_mesh = NULL;
FString test_string = "";
FString floor_material_string = "";
int test_size = -1;
UMaterial* NFMaterial = NULL;
FTimerHandle TestTimerHandle;
int draw_lidar = 1;

void UNewActorComponent_Lidar::timer_expire() {
}

void UNewActorComponent_Lidar::show_lidar() {
    if (draw_lidar == 1)
        draw_lidar = 0;
    else
        draw_lidar = 1;
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
    L = Cast<UCameraComponent> ((GetOwner()->GetComponentsByClass(UCameraComponent::StaticClass()))[1]);

    //change floor texture
    //test_size = ChangeFloorTexture(&test_string);

    // ...
    //setup the input component to capture a screenshot on a keypress
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    OwnerCharacter->InputComponent->BindAction("ScreenShotMode", IE_Pressed, this, &UNewActorComponent_Lidar::GetScreenshot);
    OwnerCharacter->InputComponent->BindAction("ShowLidar", IE_Pressed, this, &UNewActorComponent_Lidar::show_lidar);

    //setup periodic timer
    GetWorld()->GetTimerManager().SetTimer(TestTimerHandle, this, &UNewActorComponent_Lidar::ChangeWallTexture, 2.0f, true);
}

void UNewActorComponent_Lidar::ChangeWallTexture()
{
    UWorld* World = GetWorld();
    TArray<AActor*> FoundBrushes;
    FString floor_string = "Floor";
    FString brush_name;
    int num_brushes = 0;

    //get all the brushes actors
    UGameplayStatics::GetAllActorsOfClass(World, ABrush::StaticClass(), FoundBrushes);
    num_brushes = FoundBrushes.Num();

    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("Number of brushes: %d"), num_brushes));

    for (AActor* TActor: FoundBrushes) {
        if(TActor != nullptr) {
            brush_name = TActor->GetName();

            //get the components of the floor
            TArray <UBrushComponent *> Brush_Components;
            //TActor->GetComponents<UBrushComponent>(Brush_Components);

            //UMaterialInstanceDynamic* DMaterial = UMaterialInstanceDynamic::Create(Floor_Components[0]->GetMaterial(0), nullptr);
            //floor_material_string = *DMaterial->GetName(); 

            if (NFMaterial != NULL) {
                //UMaterialInstanceDynamic* NFMaterial_Instance = UMaterialInstanceDynamic::Create(NFMaterial, Brush_Components[0]);
                //set the new material to the floor
                //Brush_Components[0]->SetMaterial(0,NFMaterial_Instance);
            }
        }
    }
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
    FString UE4Str;
    FVector current_location, end_location;
    FRotator current_rotation;
    FVector current_forward_vector;
    
    if (L == NULL) {
        L = Cast<UCameraComponent> ((GetOwner()->GetComponentsByClass(UCameraComponent::StaticClass()))[1]); //Lidar needs to be the second camera in the component list
    }
    
    UE4Str = L->GetName();
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

            if (draw_lidar) {
                if (i==0)
                    DrawDebugLine(GetWorld(), current_location, impact, FColor(0,0,255), false, -1, 0, .33);
                else
                    DrawDebugLine(GetWorld(), current_location, impact, FColor(255,0,0), false, -1, 0, .33);
                DrawDebugPoint(GetWorld(), impact, 20, FColor(255,0,255), false, 0.03);
            }
            GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Red, FString::Printf(TEXT("Lidar distance ~> %s %s %d"), *test_string, *floor_material_string, test_size));
        } else {
            //there was no impact, so draw the trace in green
            if (draw_lidar) {
                DrawDebugLine(GetWorld(), current_location, end_location, FColor(0,255,0), false, -1, 0, .33);
            }
        }

    }
}

void UNewActorComponent_Lidar::GetScreenshot() {
    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("Requesting screenshot")); 
    FString fileName("/Users/jseng/screenshot.png");
    FScreenshotRequest::RequestScreenshot(fileName, false, false);
}

// Called every frame
void UNewActorComponent_Lidar::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    GetLidarScan();
}
