#include "TerminalActor.h"

#include <ThirdParty/ShaderConductor/ShaderConductor/External/DirectXShaderCompiler/include/dxc/DXIL/DxilConstants.h>

#include "AudioMixerBlueprintLibrary.h"
#include "PlayerCharacter.h"
#include "Math/UnrealMathUtility.h"
#include "Camera/CameraComponent.h"
#include "Components/WidgetComponent.h"
#include "Algo/RandomShuffle.h"
#include "TimerManager.h"

void ATerminalActor::OnFileWorkComplete()
{
	FilesRefinedCount++;

	// !!! ADD THIS LINE !!!
	// Broadcast the update so the UI knows we are at 1/2 or 2/2
	OnFileCompleted(FilesRefinedCount, FilesPerDay);

	if (FilesRefinedCount >= FilesPerDay) // Use the variable, not hardcoded '2'
	{
		bDayActive = false;
		float Duration = GetWorld()->GetTimeSeconds() - DayStartTime;

		BP_OnDayComplete(Duration);
	}
	else
	{
		ResetProgressBars();
		BP_OnShowFileSelection();
	}
}

ATerminalActor::ATerminalActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CRTMonitor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CRTMonitor"));
	RootComponent = CRTMonitor;

	// Safe default sizes (properly resized in GenerateGrid)
	const int32 InitialSize = GridWidth * GridHeight;
	GridNumbers.Init(0, InitialSize);
	ProgressBars.Init(0.f, 4);
	HighlightedPrimes.Init(false, InitialSize);
	ScaryActive.Init(false, InitialSize);
	

	TerminalCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TerminalCamera"));
	TerminalCamera->SetupAttachment(RootComponent);
	TerminalCamera->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	TerminalCamera->SetRelativeRotation(FRotator(-10.f, 180.f, 0.f));

	bBarCoolingDown.Init(false, 4);
	BarCooldownRemaining.Init(0.f, 4);

	MaxSensorDistance = 15.0f;
	
	ScrollX = 500;
	ScrollY = 500;
}

void ATerminalActor::BeginPlay()
{
	Super::BeginPlay();
	GenerateGrid();
}

/* -------------------- GRID / PRIMES -------------------- */

void ATerminalActor::GenerateGrid()
{

	UE_LOG(LogTemp, Warning, TEXT("!!! GenerateGrid is STARTING !!!"));
	
	const int32 TotalGlobalCount = GlobalMapWidth * GlobalMapHeight;
	GlobalGridNumbers.Init(0, TotalGlobalCount);
	ScaryActive.Init(false, TotalGlobalCount);

	// 1. Fill with base numbers
	for (int32 i = 0; i < TotalGlobalCount; ++i)
	{
		GlobalGridNumbers[i] = FMath::RandRange(1, 9);
	}

	// 2. Sector Spawning Logic
	int32 SectorSize = 50; 
	for (int32 y = 0; y < GlobalMapHeight; y += SectorSize)
	{
		for (int32 x = 0; x < GlobalMapWidth; x += SectorSize)
		{
			// Pick a random tile coordinate within this 50x50 block
			int32 RandX = x + FMath::RandRange(5, SectorSize - 5);
			int32 RandY = y + FMath::RandRange(5, SectorSize - 5);
			int32 GlobalIdx = RandY * GlobalMapWidth + RandX;

			if (ScaryActive.IsValidIndex(GlobalIdx))
			{
				ScaryActive[GlobalIdx] = true;
			}
			UE_LOG(LogTemp, Warning, TEXT("Scary Number spawned at Global Index: %d"), GlobalIdx);
		}
		
	}
	OnGridScrolled();

	
}

bool ATerminalActor::IsPrime(const int32 Number) const
{
	return (Number == 2 || Number == 3 || Number == 5 || Number == 7);
}

bool ATerminalActor::IsIndexScary(int32 ScreenIndex) const
{
	const int32 GlobalIdx = GetGlobalIndexFromScreenIndex(ScreenIndex);

	if (ScaryActive.IsValidIndex(GlobalIdx))
	{
		return ScaryActive[GlobalIdx];
	}
	return false;
}

void ATerminalActor::HighlightRandomPrime()
{
	if (PrimeIndices.Num() == 0)
	{
		return;
	}

	int32 ActiveCount = 0;
	for (bool bActive : ScaryActive)
	{
		if (bActive)
		{
			++ActiveCount;
		}
	}

	

	TArray<int32> Candidates;
	for (int32 Idx : PrimeIndices)
	{
		if (ScaryActive.IsValidIndex(Idx) && !ScaryActive[Idx])
		{
			Candidates.Add(Idx);
		}
	}

	if (Candidates.Num() == 0)
	{
		return;
	}

	const int32 Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	ScaryActive[Chosen] = true;
	
}

/* -------------------- PROGRESS BARS -------------------- */

void ATerminalActor::ResetProgressBars()
{
	ProgressBars.Init(0.f, 4);

	for (int32 BarIndex = 0; BarIndex < 4; ++BarIndex)
	{
		OnProgressUpdated(BarIndex, 0.f);
	}
}


bool ATerminalActor::IsBarAvailable(int32 BarIndex) const
{
	// 1. Check bounds
	if (!bBarCoolingDown.IsValidIndex(BarIndex) || !ProgressBars.IsValidIndex(BarIndex))
	{
		return false;
	}

	// 2. Check Cooldown
	bool bIsCooling = bBarCoolingDown[BarIndex];

	// 3. Check Fullness (This is the missing piece!)
	bool bIsFull = ProgressBars[BarIndex] >= 1.0f;

	// 4. Return TRUE only if it is NOT cooling AND NOT full
	return !bIsCooling && !bIsFull;
}

float ATerminalActor::GetBarCooldownRatio(int32 BarIndex) const
{
	if (!BarCooldownRemaining.IsValidIndex(BarIndex) || BarCooldownSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(BarCooldownRemaining[BarIndex] / BarCooldownSeconds, 0.f, 1.f);
}

void ATerminalActor::ApplChunkToBar(int32 BarIndex)
{
	if (!bDayActive || PendingChunkValue <= 0.f)
	{
		return;
	}

	if (!ProgressBars.IsValidIndex(BarIndex) || ProgressBars[BarIndex] >= 1.f)
	{
		return;
	}

	ProgressBars[BarIndex] = FMath::Clamp(
		ProgressBars[BarIndex] + PendingChunkValue * 1.5f,
		0.f,
		1.f
	);

	PendingChunkValue = 0.f;
	OnChunkConsumed();
	OnProgressUpdated(BarIndex, ProgressBars[BarIndex]);

	if (GetMasterProgress() >= 1.0f)
	{
		// This file is 100% refined!
		OnFileWorkComplete(); 
	}

	if (AllBarsFull())
	{
		OnAllBarsFull();
	}
}

float ATerminalActor::GetSensorProximityValue() const
{
	// 1. INCREASE THIS: This is the "Time" the bar has to grow. 
	// A larger number means it starts detecting from further away.
	// (Ideally set this in the Constructor, but you can override it here to test)
	float RealMaxDistance = 15.0f; // Was likely 5.0f or 7.0f before

	float MinDistSq = FMath::Square(RealMaxDistance);
	bool bFound = false;

	float CenterX = (float)ScrollX + AccumulatorX + 4.5f;
	float CenterY = (float)ScrollY + AccumulatorY + 4.5f;

	// 2. WIDEN THE LOOPS:
	// The loop must look far enough to cover the new RealMaxDistance.
	// We scan roughly +/- 15 tiles from the scroll position now.
	int32 SearchRadius = 16; 

	for (int32 y = ScrollY - SearchRadius; y <= ScrollY + SearchRadius + 10; ++y)
	{
		for (int32 x = ScrollX - SearchRadius; x <= ScrollX + SearchRadius + 10; ++x)
		{
			// --- WRAPPING LOGIC (Keep this exactly the same) ---
			int32 WrappedX = x % GlobalMapWidth;
			if (WrappedX < 0) WrappedX += GlobalMapWidth;

			int32 WrappedY = y % GlobalMapHeight;
			if (WrappedY < 0) WrappedY += GlobalMapHeight;

			int32 GlobalIdx = WrappedY * GlobalMapWidth + WrappedX;
			// --------------------------------------------------

			if (ScaryActive.IsValidIndex(GlobalIdx) && ScaryActive[GlobalIdx])
			{
				// Use the raw 'x' and 'y' for correct physical distance
				float DistSq = FMath::Square(x - CenterX) + FMath::Square(y - CenterY);
              
				if (DistSq < MinDistSq)
				{
					MinDistSq = DistSq;
					bFound = true;
				}
			}
		}
	}

	if (!bFound) return 0.0f;

	float Distance = FMath::Sqrt(MinDistSq);
    
	// Use the new larger distance for the math
	return FMath::Clamp(1.0f - (Distance / RealMaxDistance), 0.0f, 1.0f);
}

bool ATerminalActor::AllBarsFull() const
{
	for (float Bar : ProgressBars)
	{
		if (Bar < 1.f)
		{
			return false;
		}
	}
	return true;
}

// 1. Implementation for OnAllBarsFull (Virtual function)
void ATerminalActor::OnAllBarsFull()
{
    UE_LOG(LogTemp, Warning, TEXT("All bars are full!"));
}

void ATerminalActor::ExitTerminalMode()
{
	APlayerController* PC = GetWorld()-> GetFirstPlayerController();

	if (PC)
	{
		if (APlayerCharacter* Player = Cast<APlayerCharacter>(PC->GetPawn()))
		{
			Player->StandUpFromTerminal();
		}
	}
}

float ATerminalActor::GetMasterProgress() const
{
	float Total = 0.0;
	for (float BinProgress : ProgressBars)
	{
		Total += BinProgress;
	}

	return FMath::Clamp(Total / 4.0f, 0.0f, 1.0f);
}

// 2. Start and End Day Logic
void ATerminalActor::StartDay()
{
    bDayActive = true;

	DayStartTime = GetWorld()->GetTimeSeconds();
	
    GenerateGrid();
    OnDayStarted();
}

void ATerminalActor::EndDay()
{
    bDayActive = false;
    OnDayCompleted();
}

// 3. Input Handling
void ATerminalActor::ApplyTrackballInput(float AxisX, float AxisY)
{
	// 1. Accumulate movement
	AccumulatorX += (AxisX * -1.0f) * ScrollSensitivity;
	AccumulatorY += (AxisY * -1.0f) * ScrollSensitivity;

	// 2. Update Scroll positions
	ScrollX += (int32)AccumulatorX;
	ScrollY += (int32)AccumulatorY;

	// 3. Clear decimal accumulation
	AccumulatorX -= (int32)AccumulatorX;
	AccumulatorY -= (int32)AccumulatorY;

	// --- NEW: Wrap the Scroll variables themselves to keep them small ---
	// This creates a true "Infinite" scroll without math errors
    
	// Wrap X
	ScrollX = ScrollX % GlobalMapWidth;
	if (ScrollX < 0) ScrollX += GlobalMapWidth;

	// Wrap Y
	ScrollY = ScrollY % GlobalMapHeight;
	if (ScrollY < 0) ScrollY += GlobalMapHeight;

	// 4. Update visual grid
	OnGridScrolled();
}

// 4. Index Helpers
int32 ATerminalActor::GetGlobalIndexFromScreenIndex(int32 ScreenIndex) const
{
	// 1. Calculate Local Screen Coordinates
	int32 ScreenX = ScreenIndex % GridWidth;
	int32 ScreenY = ScreenIndex / GridWidth;

	// 2. Calculate "Raw" Global Coordinates
	int32 RawGlobalX = ScrollX + ScreenX;
	int32 RawGlobalY = ScrollY + ScreenY;

	// 3. Wrap X safely (Handles negatives correctly)
	// This creates the "Pac-Man" effect horizontally
	int32 WrappedX = RawGlobalX % GlobalMapWidth;
	if (WrappedX < 0) WrappedX += GlobalMapWidth;

	// 4. Wrap Y safely
	// This creates the "Pac-Man" effect vertically
	int32 WrappedY = RawGlobalY % GlobalMapHeight;
	if (WrappedY < 0) WrappedY += GlobalMapHeight;

	// 5. Convert back to a 1D Index
	return WrappedY * GlobalMapWidth + WrappedX;
}

int32 ATerminalActor::GetGridNumber(int32 GridX, int32 GridY) const
{
	// 1. Wrap X safely (Handles large positive numbers AND negatives)
	int32 WrappedX = GridX % GlobalMapWidth;
	if (WrappedX < 0) WrappedX += GlobalMapWidth;

	// 2. Wrap Y safely
	int32 WrappedY = GridY % GlobalMapHeight;
	if (WrappedY < 0) WrappedY += GlobalMapHeight;

	// 3. Calculate Index using the safe, wrapped coordinates
	int32 GlobalIdx = WrappedY * GlobalMapWidth + WrappedX;

	// 4. Return the value
	if (GlobalGridNumbers.IsValidIndex(GlobalIdx))
	{
		return GlobalGridNumbers[GlobalIdx];
	}
    
	return 0;
}

// 5. Scary System Grouping
TArray<int32> ATerminalActor::Get3x3Group(int32 CenterIndex) const
{
    TArray<int32> Group;
    int32 CenterX = CenterIndex % GlobalMapWidth;
    int32 CenterY = CenterIndex / GlobalMapWidth;

    for (int32 y = CenterY - 1; y <= CenterY + 1; ++y)
    {
        for (int32 x = CenterX - 1; x <= CenterX + 1; ++x)
        {
            if (x >= 0 && x < GlobalMapWidth && y >= 0 && y < GlobalMapHeight)
            {
                Group.Add(y * GlobalMapWidth + x);
            }
        }
    }
    return Group;
}

// 6. Handling the Drop
void ATerminalActor::HandleScaryDrop(const TArray<int32>& TileIndices, int32 BarIndex)
{
	float TotalValueFromSnake = 0.0f;

	for (int32 ScreenIdx : TileIndices)
	{
		// 1. Convert Screen (0-99) to Global Map index
		int32 GlobalIdx = GetGlobalIndexFromScreenIndex(ScreenIdx);
        
		// 2. Use GlobalGridNumbers (your actual array name)
		if (GlobalGridNumbers.IsValidIndex(GlobalIdx))
		{
			// Get the value of the number (1-9)
			int32 NumberValue = GlobalGridNumbers[GlobalIdx];
            
			// Base progress calculation: Higher numbers = more progress
			// Example: a '9' gives 0.045, a '1' gives 0.005
			float ProgressContribution = (float)NumberValue * 0.005f;

			// 3. Apply the "Scary" multiplier if it's red
			if (ScaryActive.IsValidIndex(GlobalIdx) && ScaryActive[GlobalIdx])
			{
				ProgressContribution *= 4.0f; // Massive bonus for red high numbers
				ScaryActive[GlobalIdx] = false; // "Consume" the red state
			}

			// 4. Refresh the tile: Replace the eaten number with a new one
			GlobalGridNumbers[GlobalIdx] = FMath::RandRange(1, 9);
            
			TotalValueFromSnake += ProgressContribution;
		}
	}

	// 5. Apply the final calculated weight to the bar
	PendingChunkValue = TotalValueFromSnake;
	ApplChunkToBar(BarIndex);

	// 6. Force the UI to refresh to show new numbers
	OnGridScrolled();
	
}

bool ATerminalActor::IsBarFull(int32 BarIndex) const
{
	if (ProgressBars.IsValidIndex(BarIndex))
	{
		return ProgressBars[BarIndex] >= 1.0f;
	}
	return false;
}
