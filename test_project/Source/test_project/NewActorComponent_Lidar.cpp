#include "NewActorComponent_Lidar.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/GameFramework/Character.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/Engine/DirectionalLight.h"
#include "Runtime/Engine/Classes/Components/DirectionalLightComponent.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/Misc/DateTime.h"
#include "Runtime/Core/Public/HAL/FileManagerGeneric.h"
#include <string>

#define NUM_LIDAR_POINTS 30

UCameraComponent* L = NULL;
AActor* floor_mesh = NULL;
FString test_string = "";
FString floor_material_string = "";
int test_size = -1;
UMaterial* NFMaterial = NULL;
FTimerHandle SunTimerHandle;
FTimerHandle ScreenshotTimerHandle;
FTimerHandle FloorTimerHandle;
FTimerHandle WallTimerHandle;
int draw_lidar = 0;
float lidar_distance[NUM_LIDAR_POINTS*2 + 1];

void UNewActorComponent_Lidar::MoveSunToRandom() {
    UWorld* World = GetWorld();
    TArray<AActor*> FoundActors;
    FString light_string = "LightSource";
    FVector current_location;
    FRotator current_rotation;
    float rand_deg = FMath::RandRange((float)0.00001,(float)1.0);
    FRotator r = FRotator(0,360.0 * rand_deg,0);

    //get all the static mesh actors
    UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), FoundActors);

    //find the light source actor
    for (AActor* TActor: FoundActors) {
        if(TActor != nullptr && (TActor->GetName()).Contains(light_string) == true) {

            ULightComponent* light = TActor->FindComponentByClass<ULightComponent>();
            current_location = light->GetComponentLocation();
            current_rotation = light->GetComponentRotation();

            //FRotator ActorRotation = YourActorReference->GetActorRotation();
              
            current_rotation.Pitch = rand_deg*360.0;
               
            TActor->SetActorRotation(current_rotation);
            //light->SetWorldLocationAndRotation(current_location, current_rotation);
            GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("found light source %f %f %f"), current_rotation.Pitch, current_rotation.Yaw, current_rotation.Roll));
        }
    }
}
    
void UNewActorComponent_Lidar::show_lidar() {
    if (draw_lidar == 1)
        draw_lidar = 0;
    else
        draw_lidar = 1;
}

void UNewActorComponent_Lidar::CreateObjects() {
    // create a new static mesh
    FString props_dir = "Content/StarterContent/JS_Props";
    FString p_dir = "/Game/StarterContent/JS_Props/";
    GetRandomObject(&props_dir); //get a random object from the directory
    props_dir.RemoveFromEnd(".uasset");
    p_dir = p_dir + props_dir + "." + props_dir;

    //skip the Material directory
    if (props_dir.Contains("Materials"))
        return;

    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("loading mesh %s "), *p_dir));

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

    //set the location and rotation of the new mesh
    FVector SpawnLoc(FMath::RandRange(-1300.0f, 2000.0f), FMath::RandRange(-800.0f, 4500.0f), 0.0f);
    FRotator SpawnRot(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);

    AStaticMeshActor* NewActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnLoc, SpawnRot, SpawnInfo);

    //load the static mesh from the path
    UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(NULL, *p_dir, NULL, LOAD_None, NULL);

    //get the components of the newly created actor
    TArray<UStaticMeshComponent*> Components;
    NewActor->GetComponents<UStaticMeshComponent>(Components);

    //set the first component to the static mesh
    Components[0]->SetMobility(EComponentMobility::Movable);
    Components[0]->SetStaticMesh(StaticMesh);
		
    return;
}

// Get random object to place in the world
//FString dirname;
void UNewActorComponent_Lidar::GetRandomObject(FString* dirname) {
    TArray<FString> FileNames;
    FFileManagerGeneric FileMgr;
    FileMgr.SetSandboxEnabled(true);// don't ask why, I don't know :P
    FString wildcard("*"); // May be "" (empty string) to search all files
    //   dirname = "Content/StarterContent/Textures/";
    FString search_path(FPaths::Combine(*FPaths::GameDir(), *dirname, *wildcard));

    FileMgr.FindFiles(FileNames, *search_path, 
	    true,  // to list files
	    true); // to skip directories

    int32 rand_index = FMath::RandRange(0,FileNames.Num()-1);
    *dirname = *FileNames[rand_index];

    //GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("searching for files %s %d"), *FPaths::GameDir(), FileNames.Num()));

    for (auto f : FileNames)
    {
	FString filename(f);
	f.RemoveFromEnd(".xml");
	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("filename %s "), *f));
    }

    FileNames.Empty();// Clear array
}


void UNewActorComponent_Lidar::PlaceRandomObject() {
    //FVector Location(0.0f, 0.0f, 0.0f);
    //FRotator Rotation(0.0f, 0.0f, 0.0f);
    //FActorSpawnParameters SpawnInfo;
    //GetWorld()->SpawnActor<AActor>(Location, Rotation, SpawnInfo);

    FString props_dir = "Content/StarterContent/Props";
    FString p_dir = *props_dir;
    GetRandomObject(&props_dir);
    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, FString::Printf(TEXT("searching for files %s "), *props_dir));




    //test spawning mesh

    //UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(NULL, TEXT("..."), NULL, LOAD_None, NULL);
    //UStaticMeshComponent* Test = ConstructObject<UStaticMeshComponent>(UStaticMeshComponent::StaticClass(), xxxx);
    //Test->SetStaticMesh(StaticMesh);
    //Test->SetWorldLocation(GetComponentLocation());
    //Test->RegisterComponentWithWorld(xxxx->GetWorld());
    //Test->AttachTo(xxxx);
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

    //move the sun position 
    GetWorld()->GetTimerManager().SetTimer(SunTimerHandle, this, &UNewActorComponent_Lidar::MoveSunToRandom, 2.0f, true);

    //get screenshots
    GetWorld()->GetTimerManager().SetTimer(ScreenshotTimerHandle, this, &UNewActorComponent_Lidar::GetScreenshot, .225f, true);
    //GetWorld()->GetTimerManager().SetTimer(ScreenshotTimerHandle, this, &UNewActorComponent_Lidar::CreateObjects, 2.0f, true);

    //change the floor texture 	
    GetWorld()->GetTimerManager().SetTimer(FloorTimerHandle, this, &UNewActorComponent_Lidar::ChangeFloorTexture, 5.0f, true);

    //change the wall texture
    GetWorld()->GetTimerManager().SetTimer(WallTimerHandle, this, &UNewActorComponent_Lidar::ChangeWallTexture, 4.0f, true);
}

//TEMPLATE Load Obj From Path
template <typename ObjClass>
static FORCEINLINE ObjClass* LoadObjFromPath(const FName& Path)
{
    if(Path == NAME_None) return NULL;
 
    return Cast<ObjClass>(StaticLoadObject( ObjClass::StaticClass(), NULL,*Path.ToString()));
}

// Load Material From Path 
static FORCEINLINE UMaterialInterface* LoadMatFromPath(const FName& Path)
{
	if(Path == NAME_None) return NULL;
 
	return LoadObjFromPath<UMaterialInterface>(Path);
}

void UNewActorComponent_Lidar::ChangeWallTexture()
{
    UWorld* World = GetWorld();
    TArray<AActor*> FoundActors;
    FString wall_string = "BrickWall";
    
    //get random floor texture
    FString wall_dir = "Content/StarterContent/Materials/Wall/";
    FString mat_dir = "/Game/StarterContent/Materials/Wall/";
    FString f_dir = *wall_dir;
    GetRandomObject(&wall_dir);
    wall_dir.RemoveFromEnd(".uasset");
    wall_dir = "Material'" + mat_dir + wall_dir + "." + wall_dir + "'";

    UMaterialInterface* u = LoadMatFromPath(*wall_dir);	

    //get all the static mesh actors
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FoundActors);

    //set the new material to the walls
    for (AActor* TActor: FoundActors) {
        if(TActor != nullptr && (TActor->GetName()).Contains(wall_string) == true) {
            //get the components of the wall
            TArray <UStaticMeshComponent *> Wall_Components;
            TActor->GetComponents<UStaticMeshComponent>(Wall_Components);

            if (NFMaterial != NULL) {
                UMaterialInstanceDynamic* NFMaterial_Instance = UMaterialInstanceDynamic::Create(u, Wall_Components[0]);
                //set the new material to the floor
                Wall_Components[0]->SetMaterial(0,NFMaterial_Instance);
            }
        }
    }
}


void UNewActorComponent_Lidar::ChangeFloorTexture() {
    UWorld* World = GetWorld();
    TArray<AActor*> FoundActors;
    FString floor_string = "Floor";

    //get all the static mesh actors
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FoundActors);

    //get random floor texture
    FString floor_dir = "Content/StarterContent/Materials/Floor/";
    FString mat_dir = "/Game/StarterContent/Materials/Floor/";
    FString f_dir = *floor_dir;
    GetRandomObject(&floor_dir);
    floor_dir.RemoveFromEnd(".uasset");
    floor_dir = "Material'" + mat_dir + floor_dir + "." + floor_dir + "'";

    UMaterialInterface* u = LoadMatFromPath(*floor_dir);	

    //find the floor mesh
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

                if (NFMaterial != NULL) {
                    UMaterialInstanceDynamic* NFMaterial_Instance = UMaterialInstanceDynamic::Create(u, Floor_Components[0]);

                    //set the new material to the floor
                    Floor_Components[0]->SetMaterial(0,NFMaterial_Instance);
                }
                //return Floor_Components.Num();
            }
    }

    //return FoundActors.Num();
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
    TraceParams.bTraceComplex = false;
    TraceParams.bTraceAsyncScene = true;
    TraceParams.bReturnPhysicalMaterial = false;

    int counter = 0;
    for(int i=-NUM_LIDAR_POINTS;i<NUM_LIDAR_POINTS+1;i++) {
        FRotator r = FRotator(0,i*1.5,0);
        end_location = current_location + (r.RotateVector(current_forward_vector) * 2500);

        //Re-initialize hit info
        FHitResult RV_Hit(ForceInit);

        //Trace out each line 
        if (GetWorld()->LineTraceSingleByChannel(RV_Hit,current_location, end_location, ECC_Pawn, TraceParams) == true) {
            //there was an impact, then draw the trace in red
            FVector impact = RV_Hit.Location;
            lidar_distance[counter] = FVector::Dist(impact, current_location);

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

        counter++;
    }
}

void UNewActorComponent_Lidar::GetScreenshot() {
    FDateTime x = FDateTime::Now();
    FString time = x.ToString() + "." + FString::FromInt(x.GetMillisecond());

    //GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Yellow, TEXT("Requesting screenshot")); 
    FString fileName("/Users/jseng/ue4/data/screenshot_" + time + ".png");
    FScreenshotRequest::RequestScreenshot(fileName, false, false);

    //get a low resolution screenshot
    if( GEngine && 0 == 5) {
	UGameViewportClient *GameViewport = GEngine->GameViewport;
	if( GameViewport ) {
	    FViewport *Viewport = GameViewport->Viewport;
	    if( Viewport ) {
		GScreenshotResolutionX = 320;
		GScreenshotResolutionY = 240;
		Viewport->TakeHighResScreenShot( );
	    }
	}
    }

    //write screen data to file
    FString SaveDirectory = FString("/Users/jseng/ue4/data/");
    FString FileName = FString("datafile.csv");
    FString TextToSave = FString("");
    
    //store the lidar data to a string
    TextToSave.Append("screenshot_" + time + FString(" "));
    for (int i=0;i<(2*NUM_LIDAR_POINTS + 1);i++) {
        TextToSave.Append( FString::SanitizeFloat(lidar_distance[i]) + FString(" "));
    }
    
    TextToSave.Append( FString("\n"));

    //check if the directory exists
    if (FPaths::DirectoryExists(SaveDirectory)) {
        //created the absolute file path
        FString AbsoluteFilePath = SaveDirectory + "/" + FileName;

        //check if the file exists
        if (FPaths::FileExists(AbsoluteFilePath)) {
            //append to the file
            FFileHelper::SaveStringToFile(TextToSave, *AbsoluteFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
        } else {
            //create a new file
            FFileHelper::SaveStringToFile(TextToSave, *AbsoluteFilePath);
        }
    }
}

// Called every frame
void UNewActorComponent_Lidar::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    GetLidarScan();
}
