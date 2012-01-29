
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "cSkeleton.h"





cSkeleton::cSkeleton()
{
	m_MobType = 51;
	GetMonsterConfig("Skeleton");
}

cSkeleton::~cSkeleton()
{
}

bool cSkeleton::IsA( const char* a_EntityType )
{
	if( strcmp( a_EntityType, "cSkeleton" ) == 0 ) return true;
	return cMonster::IsA( a_EntityType );
}

void cSkeleton::Tick(float a_Dt)
{
	cMonster::Tick(a_Dt);

	//TODO Outsource
	//TODO should do lightcheck, not daylight -> mobs in the dark don�t burn 
	if (GetWorld()->GetWorldTime() < (12000 + 1000) && GetMetaData() != BURNING ) { //if daylight
		SetMetaData(BURNING); // BURN, BABY, BURN!  >:D
	}
}

void cSkeleton::KilledBy( cEntity* a_Killer )
{
	cMonster::RandomDropItem(E_ITEM_ARROW, 0, 2);

	cMonster::RandomDropItem(E_ITEM_BONE, 0, 2);

	cMonster::KilledBy( a_Killer );
}
