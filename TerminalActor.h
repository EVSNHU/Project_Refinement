#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerminalActor.generated.h"

UCLASS()
class PROJECT_REFINEMENT_API ATerminalActor : public AActor
{
	GENERATED_BODY()
protected:
	float DayStartTime;
	// // Tracks how many files have been 100% completed
	// int32 FilesRefinedCount = 0;
	//
	// // Called when the Master Bar hits 1.0
	// void OnFileWorkComplete();
	//
	// // Event to trigger the "Success/File Select" UI in Blueprint
	// UFUNCTION(BlueprintImplementableEvent, Category = "Refinement")
	// void BP_OnShowFileSelection();
	//
	// // Event to trigger the final "Day Complete" UI in Blueprint
	// UFUNCTION(BlueprintImplementableEvent, Category = "Refinement")
	// void BP_OnDayComplete();
public:
	ATerminalActor();

	virtual void BeginPlay() override;
	virtual void OnAllBarsFull();

	UFUNCTION(BlueprintCallable, Category= "Interaction")
	void ExitTerminalMode();
	
	// Move these from protected to public
	UPROPERTY(BlueprintReadWrite, Category = "Day")
	int32 FilesRefinedCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Refinement")
	void OnFileWorkComplete();

	// Ensure these events are accessible as "Messages"
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Refinement")
	void BP_OnShowFileSelection();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Refinement")
	void BP_OnDayComplete(float DayDurationSeconds);
	
	// Event called when player interacts
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Interaction")
	void OnPlayerInteract();

	// Event called when player leaves
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="Interaction")
	void OnPlayerExit();

	/** CRT mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* CRTMonitor;
	

	/** The numeric grid */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<int32> GridNumbers;

	/** Indices of primes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<int32> PrimeIndices;

	/** Four progress bars */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<float> ProgressBars;

	// Master Progress bar
	UFUNCTION(BlueprintPure, Category = "Refinement")
	float GetMasterProgress() const;

	/** Camera on the terminal */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Terminal UI")
	class UCameraComponent* TerminalCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ScrollOffsetX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ScrollOffsetY = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<bool> HighlightedPrimes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GridHeight = 10;

	UFUNCTION(BlueprintCallable)
	void GenerateGrid();

	UFUNCTION(BlueprintCallable)
	bool IsPrime(int32 Number) const;

	UFUNCTION(BlueprintCallable)
	void HighlightRandomPrime();

	UFUNCTION(BlueprintPure)
	bool IsIndexScary(int32 ScreenIndex) const;

	UFUNCTION(BlueprintCallable)
	void ResetProgressBars();

	/* -------------------- SCARY SYSTEM -------------------- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scary")
	TArray<bool> ScaryActive;

	UPROPERTY(BlueprintReadOnly)
	TArray<int32> GridData;

	/* -------------------- COOLDOWN -------------------- */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cooldown")
	TArray<bool> bBarCoolingDown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cooldown")
	TArray<float> BarCooldownRemaining;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooldown")
	float BarCooldownSeconds = 2.5f;

	UFUNCTION(BlueprintPure, Category="Cooldown")
	bool IsBarAvailable(int32 BarIndex) const;

	UFUNCTION(BlueprintPure, Category="Cooldown")
	float GetBarCooldownRatio(int32 BarIndex) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Cooldown")
	void OnBarCooldownStarted(int32 BarIndex, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category="Cooldown")
	void OnBarCooldownEnded(int32 BarIndex);

	

	/* -------------------- DAY / FILE -------------------- */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Day")
	int32 FilesPerDay = 2;

	UPROPERTY(BlueprintReadOnly, Category="Day")
	int32 FilesCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category="Day")
	bool bDayActive = false;

	/* -------------------- STRESS -------------------- */
	

	UFUNCTION(BlueprintCallable, Category="Day")
	void StartDay();

	UFUNCTION(BlueprintCallable, Category="Day")
	void EndDay();

	/* -------------------- PROGRESS -------------------- */

	UPROPERTY(BlueprintReadOnly, Category="Progress")
	float PendingChunkValue = 0.f;

	UFUNCTION(BlueprintPure, Category="Progress")
	bool HasPendingChunk() const { return PendingChunkValue > 0.f; }

	UFUNCTION(BlueprintCallable, Category="Progress")
	void ApplChunkToBar(int32 BarIndex);

	UFUNCTION(BlueprintImplementableEvent, Category="Progress")
	void OnChunkReady(float ChunkValue);

	UFUNCTION(BlueprintImplementableEvent, Category="Progress")
	void OnChunkConsumed();

	UFUNCTION(BlueprintCallable, Category="Terminal")
	void HandleScaryDrop(const TArray<int32>& TileIndices, int32 BarIndex);

	UFUNCTION(BlueprintPure, Category = "Progress")
	bool IsBarFull(int32 BarIndex) const;

	/* -------------------- BP EVENTS -------------------- */
	

	UFUNCTION(BlueprintImplementableEvent)
	void OnGroupCleared(const TArray<int32>& GroupIndices);

	UFUNCTION(BlueprintImplementableEvent)
	void OnProgressUpdated(int32 BarIndex, float NewValue);

	UFUNCTION(BlueprintImplementableEvent, Category="Day")
	void OnDayStarted();

	UFUNCTION(BlueprintImplementableEvent, Category="Day")
	void OnFileCompleted(int32 FilesDone, int32 FilesTarget);

	UFUNCTION(BlueprintImplementableEvent, Category="Day")
	void OnDayCompleted();

	UFUNCTION(BlueprintCallable, Category="Terminal|Grid")
	TArray<int32> Get3x3Group(int32 CenterIndex) const;

	/* -------------------- GLOBAL MAP -------------------- */

	UPROPERTY()
	TArray<int32> GlobalGridNumbers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terminal|Infinite")
	int32 GlobalMapWidth = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terminal|Infinite")
	int32 GlobalMapHeight = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Terminal|Infinite")
	int32 ScrollX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Terminal|Infinite")
	int32 ScrollY = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Terminal|Infinite")
	float ScrollSensitivity = 0.5f;

	UFUNCTION(BlueprintCallable, Category="Terminal|Input")
	void ApplyTrackballInput(float AxisX, float AxisY);

	UFUNCTION(BlueprintPure, Category="Terminal|Infinite")
	int32 GetGlobalIndexFromScreenIndex(int32 ScreenIndex) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Terminal|Events")
	void OnGridScrolled();

	UFUNCTION(BlueprintPure)
	int32 GetGridNumber(int32 GridX, int32 GridY) const;

	UFUNCTION(BlueprintPure, Category = "Scary|Sensor")
	float GetSensorProximityValue() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scary|Sensor")
	float MaxSensorDistance = 20.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Terminal|Movement")
	float AccumulatorX;

	UPROPERTY(BlueprintReadOnly, Category = "Terminal|Movement")
	float AccumulatorY;
	

private:
	FTimerHandle HighlightTimerHandle;
	FTimerHandle StressTimerHandle;

	

	/** Internal helper to find the distance to the nearest active scary number */
	float GetDistanceToNearestScary() const;

	bool AllBarsFull() const;
};
