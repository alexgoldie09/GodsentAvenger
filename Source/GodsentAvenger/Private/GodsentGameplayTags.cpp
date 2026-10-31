// Alex Goldberg rights reserved

#include "GodsentGameplayTags.h"

namespace GodsentGameplayTags
{
	/** Input Tags **/
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Move, "InputTag.Move");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Look, "InputTag.Look");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_LightAttack, "InputTag.LightAttack");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Dodge, "InputTag.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Heal, "InputTag.Heal");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_SwitchTarget, "InputTag.SwitchTarget");

	UE_DEFINE_GAMEPLAY_TAG(InputTag_MustBeHeld, "InputTag.MustBeHeld");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_MustBeHeld_Block, "InputTag.MustBeHeld.Block");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_MustBeHeld_Sprint, "InputTag.MustBeHeld.Sprint");

	UE_DEFINE_GAMEPLAY_TAG(InputTag_Toggleable, "InputTag.Toggleable");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Toggleable_TargetLock, "InputTag.Toggleable.TargetLock");
	UE_DEFINE_GAMEPLAY_TAG(InputTag_Toggleable_Light, "InputTag.Toggleable.Light");

	/** Player Weapon Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_Weapon_Sword, "Player.Weapon.Sword");

	/** Player Event Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_Equip_Sword, "Player.Event.Equip.Sword");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_HitPause, "Player.Event.HitPause");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_SuccessfulBlock, "Player.Event.SuccessfulBlock");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_Parry, "Player.Event.Parry");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_SwitchTarget_Left, "Player.Event.SwitchTarget.Left");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_SwitchTarget_Right, "Player.Event.SwitchTarget.Right");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_Dodge_Takeoff, "Player.Event.Dodge.Takeoff");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_Heal_Release, "Player.Event.Heal.Release");
	UE_DEFINE_GAMEPLAY_TAG(Player_Event_Heal_HoldPoint, "Player.Event.Heal.HoldPoint");

	/** Player Ability Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Equip_Sword, "Player.Ability.Equip.Sword");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Attack_Light, "Player.Ability.Attack.Light");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_HitPause, "Player.Ability.HitPause");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Dodge, "Player.Ability.Dodge");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Block, "Player.Ability.Block");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Sprint, "Player.Ability.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Heal, "Player.Ability.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_Light, "Player.Ability.Light");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_TargetLock, "Player.Ability.TargetLock");
	UE_DEFINE_GAMEPLAY_TAG(Player_Ability_HolyRetribution, "Player.Ability.HolyRetribution");

	/** Player Cooldown Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_Cooldown_Heal, "Player.Cooldown.Heal");

	/** Player Status Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Dodging, "Player.Status.Dodging");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Blocking, "Player.Status.Blocking");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Parrying, "Player.Status.Parrying");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_FullBlock, "Player.Status.FullBlock");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Sprinting, "Player.Status.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_TargetLock, "Player.Status.TargetLock");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_StaminaRegenBlocked, "Player.Status.StaminaRegenBlocked");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_MovementLocked, "Player.Status.MovementLocked");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_Casting, "Player.Status.Casting");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_RiposteWindow, "Player.Status.RiposteWindow");
	UE_DEFINE_GAMEPLAY_TAG(Player_Status_LightActive, "Player.Status.LightActive");

	/** Player Passive Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_Passive_HitStun, "Player.Passive.HitStun");

	/** Player Set By Caller Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_SetByCaller_AttackType_Light, "Player.SetByCaller.AttackType.Light");

	/** Enemy Ability Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Melee, "Enemy.Ability.Melee");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Melee_Light, "Enemy.Ability.Melee.Light");   // quick, blockable (zombie Swing, wolf bite)
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Melee_Heavy, "Enemy.Ability.Melee.Heavy");   // slow, heavy, usually unblockable (zombie Slam)
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Ranged, "Enemy.Ability.Ranged");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_SummonEnemies, "Enemy.Ability.SummonEnemies");

	/** Enemy Weapon Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Weapon, "Enemy.Weapon");

	/** Enemy Type Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Type_Undead, "Enemy.Type.Undead");

	/** Enemy Status Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Strafing, "Enemy.Status.Strafing");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_UnderAttack, "Enemy.Status.UnderAttack");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Unblockable, "Enemy.Status.Unblockable");
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Status_Charging, "Enemy.Status.Charging");   // running in for a heavy attack (zombie SlamRun); gates a speed effect

	/** Enemy Event Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Event_SummonEnemies, "Enemy.Event.SummonEnemies");

	/** Shared Event Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_MeleeHit, "Shared.Event.MeleeHit");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_HitReact, "Shared.Event.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_SpawnProjectile, "Shared.Event.SpawnProjectile");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_DamageTaken, "Shared.Event.DamageTaken");

	/** Shared Set By Caller Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_BaseDamage, "Shared.SetByCaller.BaseDamage");
	
	/** Shared Effect Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_Effect_HitStun, "Shared.Effect.HitStun");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Effect_IgnoreDefense, "Shared.Effect.IgnoreDefense");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Effect_Melee, "Shared.Effect.Melee");

	/** Shared Ability Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_HitReact, "Shared.Ability.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Ability_Death, "Shared.Ability.Death");

	/** Shared Status Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Dead, "Shared.Status.Dead");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Front, "Shared.Status.HitReact.Front");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Back, "Shared.Status.HitReact.Back");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Left, "Shared.Status.HitReact.Left");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_HitReact_Right, "Shared.Status.HitReact.Right");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Status_Invincible, "Shared.Status.Invincible");
	
	/** Game Data Tags **/
	UE_DEFINE_GAMEPLAY_TAG(GameData_Level_GameMap, "GameData.Level.GameMap");
	UE_DEFINE_GAMEPLAY_TAG(GameData_Level_MainMenuMap, "GameData.Level.MainMenuMap");

	/** Skill Tree Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Skill, "Skill");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Dexterity_Avoidance_Hop, "Skill.Dexterity.Avoidance.Hop");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Dexterity_Avoidance_Leap, "Skill.Dexterity.Avoidance.Leap");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Dexterity_Avoidance_GracedEvasion, "Skill.Dexterity.Avoidance.GracedEvasion");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Dexterity_WeaponWielding_AttackSpeed, "Skill.Dexterity.WeaponWielding.AttackSpeed");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Dexterity_WeaponWielding_Combo, "Skill.Dexterity.WeaponWielding.Combo");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Dexterity_WeaponWielding_FlurryInHerName, "Skill.Dexterity.WeaponWielding.FlurryInHerName");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Strength_Blocking_Block, "Skill.Strength.Blocking.Block");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Strength_Blocking_Parry, "Skill.Strength.Blocking.Parry");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Strength_Blocking_HallowedRiposte, "Skill.Strength.Blocking.HallowedRiposte");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Strength_WeaponWielding_AttackDamage, "Skill.Strength.WeaponWielding.AttackDamage");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Strength_WeaponWielding_HitStun, "Skill.Strength.WeaponWielding.HitStun");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Strength_WeaponWielding_ChasteCleave, "Skill.Strength.WeaponWielding.ChasteCleave");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Constitution_Stamina_IncreasedStamina, "Skill.Constitution.Stamina.IncreasedStamina");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Constitution_Stamina_Sprint, "Skill.Constitution.Stamina.Sprint");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Constitution_Stamina_ImmortalVigor, "Skill.Constitution.Stamina.ImmortalVigor");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Constitution_Survival_IncreasedDefence, "Skill.Constitution.Survival.IncreasedDefence");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Constitution_Survival_HolyRetribution, "Skill.Constitution.Survival.HolyRetribution");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Constitution_Survival_SaintlyRevival, "Skill.Constitution.Survival.SaintlyRevival");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Fervour_Light_Light, "Skill.Fervour.Light.Light");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Fervour_Light_GreaterLight, "Skill.Fervour.Light.GreaterLight");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Fervour_Light_BlestRadiance, "Skill.Fervour.Light.BlestRadiance");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Fervour_Healing_Heal, "Skill.Fervour.Healing.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Fervour_Healing_GreaterHeal, "Skill.Fervour.Healing.GreaterHeal");
	UE_DEFINE_GAMEPLAY_TAG(Skill_Fervour_Healing_HeavenlyLife, "Skill.Fervour.Healing.HeavenlyLife");
}
