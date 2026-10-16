// Alex Goldberg rights reserved

#include "AbilitySystem/GEExecCalc/GEExecCalc_DamageTaken.h"
#include "GodsentGameplayTags.h"
#include "AbilitySystem/GodsentAttributeSet.h"

/// The attributes the damage calculation reads
struct FGodsentDamageCapture
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower)
	DECLARE_ATTRIBUTE_CAPTUREDEF(DefensePower)
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageTaken)

	FGodsentDamageCapture()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGodsentAttributeSet, AttackPower, Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGodsentAttributeSet, DefensePower, Target, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UGodsentAttributeSet, DamageTaken, Target, false)
	}
};

static const FGodsentDamageCapture& GetGodsentDamageCapture()
{
	static FGodsentDamageCapture DamageCapture;
	return DamageCapture;
}

UGEExecCalc_DamageTaken::UGEExecCalc_DamageTaken()
{
	RelevantAttributesToCapture.Add(GetGodsentDamageCapture().AttackPowerDef);
	RelevantAttributesToCapture.Add(GetGodsentDamageCapture().DefensePowerDef);
	RelevantAttributesToCapture.Add(GetGodsentDamageCapture().DamageTakenDef);
}

void UGEExecCalc_DamageTaken::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& EffectSpec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = EffectSpec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = EffectSpec.CapturedTargetTags.GetAggregatedTags();

	float SourceAttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetGodsentDamageCapture().AttackPowerDef, EvaluateParameters, SourceAttackPower);

	float BaseDamage = 0.f;
	int32 UsedLightAttackComboCount = 0;

	for (const TPair<FGameplayTag, float>& TagMagnitude : EffectSpec.SetByCallerTagMagnitudes)
	{
		if (TagMagnitude.Key.MatchesTagExact(GodsentGameplayTags::Shared_SetByCaller_BaseDamage))
			BaseDamage = TagMagnitude.Value;

		if (TagMagnitude.Key.MatchesTagExact(GodsentGameplayTags::Player_SetByCaller_AttackType_Light))
			UsedLightAttackComboCount = FMath::RoundToInt(TagMagnitude.Value);
	}

	float TargetDefensePower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetGodsentDamageCapture().DefensePowerDef, EvaluateParameters, TargetDefensePower);

	// Each hit further into the combo deals +5% (hit 1: x1.00, hit 2: x1.05, hit 3: x1.10)
	if (UsedLightAttackComboCount != 0)
	{
		const float DamageIncreasePercentLight = (UsedLightAttackComboCount - 1) * 0.05f + 1.f;
		BaseDamage *= DamageIncreasePercentLight;
	}

	// Guard against a target with no defence (the tutorial divided by zero here)
	const float FinalDamageDone = TargetDefensePower > 0.f
		? BaseDamage * SourceAttackPower / TargetDefensePower
		: BaseDamage * SourceAttackPower;

	if (FinalDamageDone > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				GetGodsentDamageCapture().DamageTakenProperty,
				EGameplayModOp::Override,
				FinalDamageDone));
	}
}
