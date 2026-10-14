// Alex Goldberg rights reserved

#pragma once

#include "CoreMinimal.h"
#include "LatentActions.h"
#include "Engine/LatentActionManager.h"
#include "GodsentTypes/GodsentEnumTypes.h"

/// Latent action behind the CountDown Blueprint node (used for cooldown timers in the UI)
class FGodsentCountDownAction : public FPendingLatentAction
{
public:
	FGodsentCountDownAction(
		float InTotalCountDownTime,
		float InUpdateInterval,
		float& InOutRemainingTime,
		EGodsentCountDownActionOutput& InCountDownOutput,
		const FLatentActionInfo& LatentInfo) :
	bNeedToCancel(false),
	TotalCountDownTime(InTotalCountDownTime),
	UpdateInterval(InUpdateInterval),
	OutRemainingTime(InOutRemainingTime),
	CountDownOutput(InCountDownOutput),
	ExecutionFunction(LatentInfo.ExecutionFunction),
	OutputLink(LatentInfo.Linkage),
	CallbackTarget(LatentInfo.CallbackTarget),
	ElapsedInterval(0.f),
	ElapsedTimeSinceStart(0.f)
	{
	}

	/// Flags the countdown to cancel on its next update
	void CancelAction();

	// ~ Begin FPendingLatentAction Interface
	virtual void UpdateOperation(FLatentResponse& Response) override;
	// ~ End FPendingLatentAction Interface

private:
	bool bNeedToCancel;
	float TotalCountDownTime;
	float UpdateInterval;
	float& OutRemainingTime;
	EGodsentCountDownActionOutput& CountDownOutput;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	float ElapsedInterval;
	float ElapsedTimeSinceStart;
};
