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

	/** Player Set By Caller Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Player_SetByCaller_AttackType_Light, "Player.SetByCaller.AttackType.Light");

	/** Enemy Ability Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Ability_Melee, "Enemy.Ability.Melee");
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

	/** Enemy Event Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Enemy_Event_SummonEnemies, "Enemy.Event.SummonEnemies");

	/** Shared Event Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_MeleeHit, "Shared.Event.MeleeHit");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_HitReact, "Shared.Event.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Event_SpawnProjectile, "Shared.Event.SpawnProjectile");

	/** Shared Set By Caller Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_SetByCaller_BaseDamage, "Shared.SetByCaller.BaseDamage");
	
	/** Shared Effect Tags **/
	UE_DEFINE_GAMEPLAY_TAG(Shared_Effect_HitStun, "Shared.Effect.HitStun");
	UE_DEFINE_GAMEPLAY_TAG(Shared_Effect_IgnoreDefense, "Shared.Effect.IgnoreDefense");

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
}
