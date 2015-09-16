/*
 * Copyright 2014 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game/magic/spells/SpellsLvl06.h"

#include "core/Application.h"
#include "core/GameTime.h"
#include "game/Damage.h"
#include "game/Entity.h"
#include "game/EntityManager.h"
#include "game/NPC.h"
#include "game/Player.h"
#include "game/Spells.h"
#include "game/magic/spells/SpellsLvl05.h"
#include "graphics/particle/ParticleEffects.h"

#include "graphics/spells/Spells05.h"
#include "physics/Collisions.h"
#include "scene/GameSound.h"
#include "scene/Interactive.h"

void RiseDeadSpell::GetTargetAndBeta(Vec3f & target, float & beta)
{
	bool displace = true;
	
	if(m_caster == PlayerEntityHandle) {
		target = player.basePosition();
		beta = player.angle.getPitch();
	} else {
		target = entities[m_caster]->pos;
		beta = entities[m_caster]->angle.getPitch();
		displace = (entities[m_caster]->ioflags & IO_NPC) == IO_NPC;
	}
	if(displace) {
		target += angleToVectorXZ(beta) * 300.f;
	}
}

RiseDeadSpell::RiseDeadSpell()
	: m_entity(EntityHandle::Invalid)
{
	
}

bool RiseDeadSpell::CanLaunch()
{
	//TODO always cancel spell even if new one can't be launched ?
	spells.endByCaster(m_caster, SPELL_RISE_DEAD);
	
	float beta;
	Vec3f target;
	
	GetTargetAndBeta(target, beta);

	if(!ARX_INTERACTIVE_ConvertToValidPosForIO(NULL, &target)) {
		ARX_SOUND_PlaySFX(SND_MAGIC_FIZZLE);
		return false;
	}

	return true;
}

void RiseDeadSpell::Launch()
{
	float beta;
	Vec3f target;
	
	GetTargetAndBeta(target, beta);
	
	m_targetPos = target;
	ARX_SOUND_PlaySFX(SND_SPELL_RAISE_DEAD, &m_targetPos);
	
	// TODO this tolive value is probably never read
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 2000000;
	m_hasDuration = true;
	m_fManaCostPerSecond = 1.2f;
	m_entity = EntityHandle::Invalid;
	
	m_fissure.Create(target, beta);
	m_fissure.SetDuration(2000, 500, 1800);
	m_fissure.SetColorBorder(Color3f(0.5, 0.5, 0.5));
	m_fissure.SetColorRays1(Color3f(0.5, 0.5, 0.5));
	m_fissure.SetColorRays2(Color3f(1.f, 0.f, 0.f));
	
	if(!lightHandleIsValid(m_light)) {
		m_light = GetFreeDynLight();
	}
	if(lightHandleIsValid(m_light)) {
		EERIE_LIGHT * light = lightHandleGet(m_light);
		
		light->intensity = 1.3f;
		light->fallend = 450.f;
		light->fallstart = 380.f;
		light->rgb = Color3f::black;
		light->pos = target - Vec3f(0.f, 100.f, 0.f);
		light->duration = 200;
		light->time_creation = (unsigned long)(arxtime);
	}
	
	m_duration = m_fissure.GetDuration();
}

void RiseDeadSpell::End()
{
	if(ValidIONum(m_entity) && m_entity != PlayerEntityHandle) {
		
		ARX_SOUND_PlaySFX(SND_SPELL_ELECTRIC, &entities[m_entity]->pos);
		
		Entity *entity = entities[m_entity];

		if(entity->scriptload && (entity->ioflags & IO_NOSAVE)) {
			AddRandomSmoke(entity,100);
			Vec3f posi = entity->pos;
			posi.y-=100.f;
			MakeCoolFx(posi);
			
			LightHandle nn = GetFreeDynLight();

			if(lightHandleIsValid(nn)) {
				EERIE_LIGHT * light = lightHandleGet(nn);
				
				light->intensity = Random::getf(0.7f, 2.7f);
				light->fallend = 600.f;
				light->fallstart = 400.f;
				light->rgb = Color3f(1.0f, 0.8f, 0.f);
				light->pos = posi;
				light->duration = 600;
			}

			entity->destroyOne();
		}
	}
	
	endLightDelayed(m_light, 500);
}

void RiseDeadSpell::Update(float timeDelta) {
	
	if(m_entity == -2) {
		m_light = LightHandle::Invalid;
		return;
	}
	
	m_duration+=200;
	
	m_fissure.Update(timeDelta);
	m_fissure.Render();
	
	if(lightHandleIsValid(m_light)) {
		EERIE_LIGHT * light = lightHandleGet(m_light);
		
		light->intensity = 0.7f + 2.3f;
		light->fallend = 500.f;
		light->fallstart = 400.f;
		light->rgb = Color3f(0.8f, 0.2f, 0.2f);
		light->duration=800;
		light->time_creation = (unsigned long)(arxtime);
	}
	
	unsigned long tim = m_fissure.ulCurrentTime;
	
	if(tim > 3000 && m_entity == EntityHandle::Invalid) {
		ARX_SOUND_PlaySFX(SND_SPELL_ELECTRIC, &m_targetPos);
		
		Cylinder phys;
		phys.height = -200;
		phys.radius = 50;
		phys.origin = m_targetPos;
		
		float anything = CheckAnythingInCylinder(phys, NULL, CFLAG_JUST_TEST);
		
		if(glm::abs(anything) < 30) {
			
			const char * cls = "graph/obj3d/interactive/npc/undead_base/undead_base";
			Entity * io = AddNPC(cls, -1, IO_IMMEDIATELOAD);
			
			if(io) {
				ARX_INTERACTIVE_HideGore(io);
				RestoreInitialIOStatusOfIO(io);
				
				io->summoner = m_caster;
				
				io->ioflags|=IO_NOSAVE;
				m_entity = io->index();
				io->scriptload=1;
				
				ARX_INTERACTIVE_Teleport(io, phys.origin);
				SendInitScriptEvent(io);
				
				if(ValidIONum(m_caster)) {
					EVENT_SENDER = entities[m_caster];
				} else {
					EVENT_SENDER = NULL;
				}
				
				SendIOScriptEvent(io,SM_SUMMONED);
					
				Vec3f pos = m_fissure.m_eSrc;
				pos += randomVec3f() * 100.f;
				pos += Vec3f(-50.f, 50.f, -50.f);
				
				MakeCoolFx(pos);
			}
			
			m_light = LightHandle::Invalid;
		} else {
			ARX_SOUND_PlaySFX(SND_MAGIC_FIZZLE);
			m_entity = EntityHandle(-2); // FIXME inband signaling
			m_duration=0;
		}
	} else if(!arxtime.is_paused() && tim < 4000) {
	  if(Random::getf() > 0.95f) {
			MakeCoolFx(m_fissure.m_eSrc);
		}
	}
}

void ParalyseSpell::Launch()
{
	ARX_SOUND_PlaySFX(SND_SPELL_PARALYSE, &entities[m_target]->pos);
	
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 5000;
	
	float resist_magic = 0.f;
	if(m_target == PlayerEntityHandle && m_level <= player.level) {
		resist_magic = player.m_misc.resistMagic;
	} else if(entities[m_target]->ioflags & IO_NPC) {
		resist_magic = entities[m_target]->_npcdata->resist_magic;
	}
	if(Random::getf(0.f, 100.f) < resist_magic) {
		float mul = std::max(0.5f, 1.f - (resist_magic * 0.005f));
		m_duration = long(m_duration * mul);
	}
	
	entities[m_target]->ioflags |= IO_FREEZESCRIPT;
	
	m_targets.push_back(m_target);
	ARX_NPC_Kill_Spell_Launch(entities[m_target]);
}

void ParalyseSpell::End()
{
	m_targets.clear();
	entities[m_target]->ioflags &= ~IO_FREEZESCRIPT;
	
	ARX_SOUND_PlaySFX(SND_SPELL_PARALYSE_END);
}

Vec3f ParalyseSpell::getPosition() {
	return getTargetPosition();
}

CreateFieldSpell::CreateFieldSpell()
	: m_entity(EntityHandle::Invalid)
{
}


void CreateFieldSpell::Launch()
{
	unsigned long start = (unsigned long)(arxtime);
	if(m_flags & SPELLCAST_FLAG_RESTORE) {
		start -= std::min(start, 4000ul);
	}
	m_timcreation = start;
	
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 800000;
	m_hasDuration = true;
	m_fManaCostPerSecond = 1.2f;
	
	Vec3f target;
	float beta = 0.f;
	bool displace = false;
	if(m_caster == PlayerEntityHandle) {
		target = entities.player()->pos;
		beta = player.angle.getPitch();
		displace = true;
	} else {
		if(ValidIONum(m_caster)) {
			Entity * io = entities[m_caster];
			target = io->pos;
			beta = io->angle.getPitch();
			displace = (io->ioflags & IO_NPC) == IO_NPC;
		} else {
			ARX_DEAD_CODE();
		}
	}
	if(displace) {
		target += angleToVectorXZ(beta) * 250.f;
	}
	
	ARX_SOUND_PlaySFX(SND_SPELL_CREATE_FIELD, &target);
	
	res::path cls = "graph/obj3d/interactive/fix_inter/blue_cube/blue_cube";
	Entity * io = AddFix(cls, -1, IO_IMMEDIATELOAD);
	if(io) {
		
		ARX_INTERACTIVE_HideGore(io);
		RestoreInitialIOStatusOfIO(io);
		m_entity = io->index();
		io->scriptload = 1;
		io->ioflags |= IO_NOSAVE | IO_FIELD;
		io->initpos = io->pos = target;
		SendInitScriptEvent(io);
		
		m_field.Create(target);
		m_field.SetDuration(m_duration);
		m_field.lLightId = GetFreeDynLight();
		
		if(lightHandleIsValid(m_field.lLightId)) {
			EERIE_LIGHT * light = lightHandleGet(m_field.lLightId);
			
			light->intensity = 0.7f + 2.3f;
			light->fallend = 500.f;
			light->fallstart = 400.f;
			light->rgb = Color3f(0.8f, 0.0f, 1.0f);
			light->pos = m_field.eSrc - Vec3f(0.f, 150.f, 0.f);
		}
		
		m_duration = m_field.GetDuration();
		
		if(m_flags & SPELLCAST_FLAG_RESTORE) {
			m_field.Update(4000);
		}
		
	} else {
		m_duration = 0;
	}
}

void CreateFieldSpell::End() {
	
	endLightDelayed(m_field.lLightId, 800);
	
	if(ValidIONum(m_entity)) {
		delete entities[m_entity];
	}
}

void CreateFieldSpell::Update(float timeDelta) {
	
		if(ValidIONum(m_entity)) {
			Entity * io = entities[m_entity];
			
			io->pos = m_field.eSrc;

			if (IsAnyNPCInPlatform(io))
			{
				m_duration=0;
			}
		
			m_field.Update(timeDelta);
			m_field.Render();
		}
}

Vec3f CreateFieldSpell::getPosition() {
	
	return m_field.eSrc;
}

void DisarmTrapSpell::Launch()
{
	ARX_SOUND_PlaySFX(SND_SPELL_DISARM_TRAP);
	
	m_duration = 1;
	
	Sphere sphere;
	sphere.origin = player.pos;
	sphere.radius = 400.f;
	
	for(size_t n = 0; n < MAX_SPELLS; n++) {
		SpellBase * spell = spells[SpellHandle(n)];
		
		if(!spell || spell->m_type != SPELL_RUNE_OF_GUARDING) {
			continue;
		}
		
		Vec3f pos = static_cast<RuneOfGuardingSpell *>(spell)->getPosition();
		
		if(sphere.contains(pos)) {
			spell->m_level -= m_level;
			if(spell->m_level <= 0) {
				spells.endSpell(spell);
			}
		}
	}
}


bool SlowDownSpell::CanLaunch() {
	
	// TODO this seems to be the only spell that ends itself when cast twice
	SpellBase * spell = spells.getSpellOnTarget(m_target, SPELL_SLOW_DOWN);
	if(spell) {
		spells.endSpell(spell);
		return false;
	}
	
	return true;
}

void SlowDownSpell::Launch()
{
	ARX_SOUND_PlaySFX(SND_SPELL_SLOW_DOWN, &entities[m_target]->pos);
	
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 10000;
	
	if(m_caster == PlayerEntityHandle)
		m_duration = 10000000;
	
	m_hasDuration = true;
	m_fManaCostPerSecond = 1.2f;
	
	m_targets.push_back(m_target);
}

void SlowDownSpell::End() {
	
	ARX_SOUND_PlaySFX(SND_SPELL_SLOW_DOWN_END);
	m_targets.clear();
}

void SlowDownSpell::Update(float timeDelta) {
	
	ARX_UNUSED(timeDelta);
}

Vec3f SlowDownSpell::getPosition() {
	
	return getTargetPosition();
}
