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

#include "game/magic/spells/SpellsLvl07.h"

#include <glm/gtc/random.hpp>

#include "core/Application.h"
#include "core/Config.h"
#include "core/Core.h"
#include "core/GameTime.h"
#include "game/Damage.h"
#include "game/Entity.h"
#include "game/EntityManager.h"
#include "game/Player.h"
#include "game/Spells.h"
#include "game/spell/FlyingEye.h"

#include "graphics/particle/ParticleEffects.h"

#include "gui/Interface.h"

#include "scene/GameSound.h"
#include "scene/Interactive.h"
#include "scene/Object.h"
#include "scene/Scene.h"

extern float SLID_START;
bool bOldLookToggle;

static LightHandle special[3];

FlyingEyeSpell::FlyingEyeSpell()
	: m_lastupdate(0)
{
	
}

bool FlyingEyeSpell::CanLaunch()
{
	if(eyeball.exist)
		return false;

	if(spells.ExistAnyInstanceForThisCaster(m_type, m_caster))
		return false;
	
	if(m_caster == PlayerEntityHandle) {
		m_target = PlayerEntityHandle;
	}
	
	if(m_target != PlayerEntityHandle) {
		return false;
	}
	
	return true;
}

void FlyingEyeSpell::Launch()
{
	static TextureContainer * tc4 = TextureContainer::Load("graph/particles/smoke");
	
	ARX_SOUND_PlaySFX(SND_SPELL_EYEBALL_IN);
	
	m_lastupdate = m_timcreation;
	m_duration = 1000000;
	m_hasDuration = true;
	m_fManaCostPerSecond = 3.2f;
	eyeball.exist = 1;
	
	eyeball.pos = player.pos;
	eyeball.pos += angleToVectorXZ(player.angle.getPitch()) * 200.f;
	eyeball.pos += Vec3f(0.f, 50.f, 0.f);
	
	eyeball.angle = player.angle;
	
	for(long n = 0; n < 12; n++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			break;
		}
		
		pd->ov = eyeball.pos + randomVec(-5.f, 5.f);
		pd->move = randomVec(-2.f, 2.f);
		pd->siz = 28.f;
		pd->tolive = Random::get(2000, 6000);
		pd->scale = Vec3f(12.f);
		pd->tc = tc4;
		pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING;
		pd->fparam = 0.0000001f;
		pd->rgb = Color3f(0.7f, 0.7f, 1.f);
	}
	
	TRUE_PLAYER_MOUSELOOK_ON = true;
	SLID_START = float(arxtime);
	bOldLookToggle = config.input.mouseLookToggle;
	config.input.mouseLookToggle = true;
}

void FlyingEyeSpell::End()
{
	ARX_SOUND_PlaySFX(SND_MAGIC_FIZZLE, &entities[m_caster]->pos);
	
	static TextureContainer * tc4=TextureContainer::Load("graph/particles/smoke");
	
	ARX_SOUND_PlaySFX(SND_SPELL_EYEBALL_OUT);
	eyeball.exist = -100;
	
	for(long n = 0; n < 12; n++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			break;
		}
		
		pd->ov = eyeball.pos + randomVec(-5.f, 5.f);
		pd->move = randomVec(-2.f, 2.f);
		pd->siz = 28.f;
		pd->tolive = Random::get(2000, 6000);
		pd->scale = Vec3f(12.f);
		pd->tc = tc4;
		pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING;
		pd->fparam = 0.0000001f;
		pd->rgb = Color3f(0.7f, 0.7f, 1.f);
	}
	
	config.input.mouseLookToggle = bOldLookToggle;
	
	lightHandleDestroy(special[2]);
	lightHandleDestroy(special[1]);
}

void FlyingEyeSpell::Update(float timeDelta)
{
	ARX_UNUSED(timeDelta);
	
	const unsigned long tim = (unsigned long)(arxtime);
	
	const long framediff3 = tim - m_lastupdate;
	
	eyeball.floating = std::sin(m_lastupdate-m_timcreation * 0.001f);
	eyeball.floating *= 10.f;
	
	if(m_lastupdate-m_timcreation <= 3000) {
		eyeball.exist = m_lastupdate - m_timcreation * (1.0f / 30);
		eyeball.size = Vec3f(1.f - float(eyeball.exist) * 0.01f);
		eyeball.angle.setPitch(eyeball.angle.getPitch() + framediff3 * 0.6f);
	} else {
		eyeball.exist = 2;
	}
	
	m_lastupdate=tim;
	
	Entity * io = entities.player();
	EERIE_3DOBJ * eobj = io->obj;
	long pouet = 2;

	while(pouet) {
		long id;

		if(pouet == 2)
			id = io->obj->fastaccess.primary_attach;
		else
			id = GetActionPointIdx(io->obj, "left_attach");

		pouet--;

		if(id != -1) {
			if(!lightHandleIsValid(special[pouet])) {
				special[pouet] = GetFreeDynLight();
			}
			if(lightHandleIsValid(special[pouet])) {
				EERIE_LIGHT * el = lightHandleGet(special[pouet]);
				el->intensity = 1.3f;
				el->fallend = 180.f;
				el->fallstart = 50.f;
				el->rgb = Color3f(0.7f, 0.3f, 1.f);
				el->pos = eobj->vertexlist3[id].v;
			}
			
			for(long kk = 0; kk < 2; kk++) {
				
				PARTICLE_DEF * pd = createParticle();
				if(!pd) {
					break;
				}
				
				pd->ov = eobj->vertexlist3[id].v + randomVec(-1.f, 1.f);
				pd->move = Vec3f(0.1f, 0.f, 0.1f) + Vec3f(-0.2f, -2.2f, -0.2f) * randomVec3f();
				pd->siz = 5.f;
				pd->tolive = Random::get(1500, 3500);
				pd->scale = Vec3f(0.2f);
				pd->tc = TC_smoke;
				pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING;
				pd->sourceionum = PlayerEntityHandle;
				pd->source = &eobj->vertexlist3[id].v;
				pd->fparam = 0.0000001f;
				pd->rgb = Color3f(.7f, .3f, 1.f) + Color3f(-.1f, -.1f, -.1f) * randomColor3f();
			}
		}
	}
}

Vec3f FlyingEyeSpell::getPosition() {
	return eyeball.pos;
}

FireFieldSpell::FireFieldSpell()
	: m_light(LightHandle::Invalid)
	, m_damage(DamageHandle::Invalid)
{
}

void FireFieldSpell::Launch() {
	
	spells.endByCaster(m_caster, SPELL_FIRE_FIELD);
	
	ARX_SOUND_PlaySFX(SND_SPELL_FIRE_FIELD_START);
	
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 100000;
	m_hasDuration = true;
	m_fManaCostPerSecond = 2.8f;
	m_light = LightHandle::Invalid;
	
	Vec3f target;
	float beta = 0.f;
	bool displace = false;
	if(m_caster == PlayerEntityHandle) {
		target = player.basePosition();
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
	
	m_pos = target + Vec3f(0, -10, 0);
	
	DamageParameters damage;
	damage.radius = 150.f;
	damage.damages = 10.f;
	damage.area = DAMAGE_FULL;
	damage.duration = 100000000;
	damage.source = m_caster;
	damage.flags = 0;
	damage.type = DAMAGE_TYPE_MAGICAL | DAMAGE_TYPE_FIRE | DAMAGE_TYPE_FIELD;
	damage.pos = target;
	m_damage = DamageCreate(damage);
	
	m_snd_loop = ARX_SOUND_PlaySFX(SND_SPELL_FIRE_FIELD_LOOP, &target, 1.f, ARX_SOUND_PLAY_LOOPED);
	
	{
	ParticleParams cp = ParticleParams();
	cp.m_nbMax = 100;
	cp.m_life = 2000;
	cp.m_lifeRandom = 1000;
	cp.m_pos = Vec3f(80, 10, 80);
	cp.m_direction = Vec3f(0.f, 1.f, 0.f);
	cp.m_angle = 0;
	cp.m_speed = 0;
	cp.m_speedRandom = 0;
	cp.m_gravity = Vec3f_ZERO;
	cp.m_flash = 0;
	cp.m_rotation = 0;
	cp.m_rotationRandomDirection = false;
	cp.m_rotationRandomStart = false;

	cp.m_startSegment.m_size = 10;
	cp.m_startSegment.m_sizeRandom = 3;
	cp.m_startSegment.m_color = Color(25, 25, 25, 50).to<float>();
	cp.m_startSegment.m_colorRandom = Color(51, 51, 51, 101).to<float>();

	cp.m_endSegment.m_size = 10;
	cp.m_endSegment.m_sizeRandom = 3;
	cp.m_endSegment.m_color = Color(25, 25, 25, 50).to<float>();
	cp.m_endSegment.m_colorRandom = Color(0, 0, 0, 100).to<float>();
	cp.m_texture.m_texLoop = true;

	cp.m_blendMode = RenderMaterial::AlphaAdditive;
	cp.m_freq = 150.0f;
	cp.m_texture.set("graph/particles/firebase", 4, 100);
	cp.m_spawnFlags = 0;
	
	pPSStream.SetParams(cp);
	}
	pPSStream.SetPos(m_pos);
	pPSStream.Update(0);

	//-------------------------------------------------------------------------

	{
	ParticleParams cp = ParticleParams();
	cp.m_nbMax = 50;
	cp.m_life = 1000;
	cp.m_lifeRandom = 500;
	cp.m_pos = Vec3f(100, 10, 100);
	cp.m_direction = Vec3f(0.f, -1.f, 0.f);
	cp.m_angle = glm::radians(10.f);
	cp.m_speed = 0;
	cp.m_speedRandom = 0;
	cp.m_gravity = Vec3f_ZERO;
	cp.m_flash = 0;
	cp.m_rotation = 0;
	cp.m_rotationRandomDirection = false;
	cp.m_rotationRandomStart = false;

	cp.m_startSegment.m_size = 10;
	cp.m_startSegment.m_sizeRandom = 10;
	cp.m_startSegment.m_color = Color(40, 40, 40, 50).to<float>();
	cp.m_startSegment.m_colorRandom = Color(51, 51, 51, 100).to<float>();

	cp.m_endSegment.m_size = 10;
	cp.m_endSegment.m_sizeRandom = 10;
	cp.m_endSegment.m_color = Color(0, 0, 0, 50).to<float>();
	cp.m_endSegment.m_colorRandom = Color(0, 0, 0, 100).to<float>();
	cp.m_texture.m_texLoop = false;

	cp.m_blendMode = RenderMaterial::Additive;
	cp.m_freq = 150.0f;
	cp.m_texture.set("graph/particles/fire", 0, 500);
	cp.m_spawnFlags = 0;
	
	pPSStream1.SetParams(cp);
	}
	pPSStream1.SetPos(m_pos + Vec3f(0, 10, 0));
	pPSStream1.Update(0);
}

void FireFieldSpell::End()
{
	DamageRequestEnd(m_damage);
	
	ARX_SOUND_Stop(m_snd_loop);
	ARX_SOUND_PlaySFX(SND_SPELL_FIRE_FIELD_END);
}

void FireFieldSpell::Update(float timeDelta) {
	
	pPSStream.Update(timeDelta);
	pPSStream1.Update(timeDelta);
	
	
	if(!lightHandleIsValid(m_light))
		m_light = GetFreeDynLight();
	
	if(lightHandleIsValid(m_light)) {
		EERIE_LIGHT * el = lightHandleGet(m_light);
		
		el->pos = m_pos + Vec3f(0.f, -120.f, 0.f);
		el->intensity = 4.6f;
		el->fallstart = Random::getf(150.f, 180.f);
		el->fallend   = Random::getf(290.f, 320.f);
		el->rgb = Color3f(1.f, 0.8f, 0.6f) + Color3f(Random::getf(-0.1f, 0.f), 0.f, 0.f);
		el->duration = 600;
		el->extras=0;
	}
	
	if(VisibleSphere(Sphere(m_pos - Vec3f(0.f, 120.f, 0.f), 350.f))) {
		
		pPSStream.Render();
		pPSStream1.Render();
		
		float fDiff = timeDelta / 8.f;
		int nTime = checked_range_cast<int>(fDiff);
		
		for(long nn=0;nn<=nTime+1;nn++) {
			
			PARTICLE_DEF * pd = createParticle();
			if(!pd) {
				break;
			}
			
			float t = Random::getf() * (PI * 2.f) - PI;
			float ts = std::sin(t);
			float tc = std::cos(t);
			pd->ov = m_pos + Vec3f(120.f * ts, 15.f * ts, 120.f * tc) * randomVec();
			pd->move = Vec3f(2.f, 1.f, 2.f) + Vec3f(-4.f, -8.f, -4.f) * randomVec3f();
			pd->siz = 7.f;
			pd->tolive = Random::get(500, 1500);
			pd->tc = fire2;
			pd->special = ROTATING | MODULATE_ROTATION | FIRE_TO_SMOKE;
			pd->fparam = Random::getf(-0.1f, 0.1f);
			pd->scale = Vec3f(-8.f);
			
			PARTICLE_DEF * pd2 = createParticle();
			if(!pd2) {
				break;
			}
			
			*pd2 = *pd;
			pd2->delay = Random::get(60, 210);
		}
		
	}
}

Vec3f FireFieldSpell::getPosition() {
	
	return m_pos;
}



IceFieldSpell::IceFieldSpell()
	: SpellBase()
	, tex_p1(NULL)
	, tex_p2(NULL)
{}

void IceFieldSpell::Launch()
{
	spells.endByCaster(m_caster, SPELL_ICE_FIELD);
	
	ARX_SOUND_PlaySFX(SND_SPELL_ICE_FIELD);
	
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 100000;
	m_hasDuration = true;
	m_fManaCostPerSecond = 2.8f;
	m_light = LightHandle::Invalid;
	
	Vec3f target;
	float beta = 0.f;
	bool displace = false;
	if(m_caster == PlayerEntityHandle) {
		target = player.basePosition();
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
	
	m_pos = target;
	
	DamageParameters damage;
	damage.radius = 150.f;
	damage.damages = 10.f;
	damage.area = DAMAGE_FULL;
	damage.duration = 100000000;
	damage.source = m_caster;
	damage.flags = 0;
	damage.type = DAMAGE_TYPE_MAGICAL | DAMAGE_TYPE_COLD | DAMAGE_TYPE_FIELD;
	damage.pos = target;
	m_damage = DamageCreate(damage);
	
	tex_p1 = TextureContainer::Load("graph/obj3d/textures/(fx)_tsu_blueting");
	tex_p2 = TextureContainer::Load("graph/obj3d/textures/(fx)_tsu_bluepouf");
	
	for(int i = 0; i < iMax; i++) {
		float t = Random::getf();

		if (t < 0.5f)
			tType[i] = 0;
		else
			tType[i] = 1;
		
		tSize[i] = Vec3f_ZERO;
		tSizeMax[i] = randomVec3f() + Vec3f(0.f, 0.2f, 0.f);
		
		Vec3f minPos;
		if(tType[i] == 0) {
			minPos = Vec3f(1.2f, 1, 1.2f);
		} else {
			minPos = Vec3f(0.4f, 0.3f, 0.4f);
		}
		
		tSizeMax[i] = glm::max(tSizeMax[i], minPos);
		
		if(tType[i] == 0) {
			tPos[i].x = m_pos.x + Random::getf(-80.f, 80.f);
			tPos[i].y = m_pos.y;
			tPos[i].z = m_pos.z + Random::getf(-80.f, 80.f);
		} else {
			tPos[i].x = m_pos.x + Random::getf(-120.f, 120.f);
			tPos[i].y = m_pos.y;
			tPos[i].z = m_pos.z + Random::getf(-120.f, 120.f);
		}
	}
	
	m_snd_loop = ARX_SOUND_PlaySFX(SND_SPELL_ICE_FIELD_LOOP, &target, 1.f, ARX_SOUND_PLAY_LOOPED);
}

void IceFieldSpell::End() {
	
	DamageRequestEnd(m_damage);
	
	ARX_SOUND_Stop(m_snd_loop); 
	ARX_SOUND_PlaySFX(SND_SPELL_ICE_FIELD_END);
}

void IceFieldSpell::Update(float timeDelta) {
	
	ARX_UNUSED(timeDelta);
	
	if(!lightHandleIsValid(m_light))
		m_light = GetFreeDynLight();

	if(lightHandleIsValid(m_light)) {
		EERIE_LIGHT * el = lightHandleGet(m_light);
		
		el->pos = m_pos + Vec3f(0.f, -120.f, 0.f);
		el->intensity = 4.6f;
		el->fallstart = Random::getf(150.f, 180.f);
		el->fallend   = Random::getf(290.f, 320.f);
		el->rgb = Color3f(0.76f, 0.76f, 1.0f) + Color3f(0.f, 0.f, Random::getf(-0.1f, 0.f));
		el->duration = 600;
		el->extras=0;
	}

	if(!VisibleSphere(Sphere(m_pos - Vec3f(0.f, 120.f, 0.f), 350.f)))
		return;
	
	RenderMaterial mat;
	mat.setDepthTest(true);
	mat.setBlendType(RenderMaterial::Additive);
	
	for(int i = 0; i < iMax; i++) {
		
		tSize[i] += Vec3f(0.1f);
		tSize[i] = glm::min(tSize[i], tSizeMax[i]);
		
		Anglef stiteangle = Anglef::ZERO;
		Vec3f stitepos;
		Vec3f stitescale;
		Color3f stitecolor;

		stiteangle.setPitch(glm::cos(glm::radians(tPos[i].x)) * 360);
		stitepos.x = tPos[i].x;
		stitepos.y = m_pos.y;
		stitepos.z = tPos[i].z;
		
		stitecolor.r = tSizeMax[i].y * 0.7f;
		stitecolor.g = tSizeMax[i].y * 0.7f;
		stitecolor.b = tSizeMax[i].y * 0.9f;

		if(stitecolor.r > 1)
			stitecolor.r = 1;

		if(stitecolor.g > 1)
			stitecolor.g = 1;

		if(stitecolor.b > 1)
			stitecolor.b = 1;

		stitescale.z = tSize[i].x;
		stitescale.y = tSize[i].y;
		stitescale.x = tSize[i].z;

		EERIE_3DOBJ * obj = (tType[i] == 0) ? smotte : stite;
		
		Draw3DObject(obj, stiteangle, stitepos, stitescale, stitecolor, mat);
	}
	
	for(int i = 0; i < iMax * 0.5f; i++) {
		
		float t = Random::getf();
		if(t < 0.01f) {
			
			PARTICLE_DEF * pd = createParticle();
			if(pd) {
				pd->ov = tPos[i] + randomVec(-5.f, 5.f);
				pd->move = randomVec(-2.f, 2.f);
				pd->siz = 20.f;
				pd->tolive = Random::get(2000, 6000);
				pd->tc = tex_p2;
				pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING;
				pd->fparam = 0.0000001f;
				pd->rgb = Color3f(0.7f, 0.7f, 1.f);
			}
			
		} else if (t > 0.095f) {
			
			PARTICLE_DEF * pd = createParticle();
			if(pd) {
				pd->ov = tPos[i] + randomVec(-5.f, 5.f) + Vec3f(0.f, 50.f, 0.f);
				pd->move = Vec3f(0.f, Random::getf(-2.f, 2.f), 0.f);
				pd->siz = 0.5f;
				pd->tolive = Random::get(2000, 6000);
				pd->tc = tex_p1;
				pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING;
				pd->fparam = 0.0000001f;
				pd->rgb = Color3f(0.7f, 0.7f, 1.f);
			}
			
		}
	}
}

Vec3f IceFieldSpell::getPosition() {
	return m_pos;
}


void LightningStrikeSpell::Launch() {
	
	Vec3f target(0.f, 0.f, -500.f);
	m_lightning.Create(Vec3f_ZERO, target);
	m_lightning.SetDuration(long(500 * m_level));
	m_lightning.m_isMassLightning = false;
	m_duration = m_lightning.GetDuration();
	
	ARX_SOUND_PlaySFX(SND_SPELL_LIGHTNING_START, &m_caster_pos);
	
	m_snd_loop = ARX_SOUND_PlaySFX(SND_SPELL_LIGHTNING_LOOP, &m_caster_pos, 1.f, ARX_SOUND_PLAY_LOOPED);
}

void LightningStrikeSpell::End()
{
	ARX_SOUND_PlaySFX(SND_SPELL_ELECTRIC, &entities[m_caster]->pos);
	
	ARX_SOUND_Stop(m_snd_loop);
	ARX_SOUND_PlaySFX(SND_SPELL_LIGHTNING_END, &entities[m_caster]->pos);
}

static Vec3f GetChestPos(EntityHandle num) {
	
	if(num == 0) {
		return player.pos + Vec3f(0.f, 70.f, 0.f);
	}

	if(ValidIONum(num)) {
		long idx = GetGroupOriginByName(entities[num]->obj, "chest");

		if(idx >= 0) {
			return entities[num]->obj->vertexlist3[idx].v;
		} else {
			return entities[num]->pos + Vec3f(0.f, -120.f, 0.f);
		}
	} else {
		// should not happen
		return Vec3f_ZERO;
	}
}

void LightningStrikeSpell::Update(float timeDelta) {
	
	float fBeta = 0.f;
	float falpha = 0.f;
	
	Entity * caster = entities[m_caster];
	long idx = GetGroupOriginByName(caster->obj, "chest");
	if(idx >= 0) {
		m_caster_pos = caster->obj->vertexlist3[idx].v;
	} else {
		m_caster_pos = caster->pos;
	}
	
	if(m_caster == PlayerEntityHandle) {
		falpha = -player.angle.getYaw();
		fBeta = player.angle.getPitch();
	} else {
		fBeta = caster->angle.getPitch();
		if(ValidIONum(caster->targetinfo) && caster->targetinfo != m_caster) {
			const Vec3f & p1 = m_caster_pos;
			Vec3f p2 = GetChestPos(caster->targetinfo);
			falpha = MAKEANGLE(glm::degrees(getAngle(p1.y, p1.z, p2.y, p2.z + glm::distance(Vec2f(p2.x, p2.z), Vec2f(p1.x, p1.z))))); //alpha entre orgn et dest;
		} else if(ValidIONum(m_target)) {
			const Vec3f & p1 = m_caster_pos;
			Vec3f p2 = GetChestPos(m_target);
			falpha = MAKEANGLE(glm::degrees(getAngle(p1.y, p1.z, p2.y, p2.z + glm::distance(Vec2f(p2.x, p2.z), Vec2f(p1.x, p1.z))))); //alpha entre orgn et dest;
		}
	}
	
	m_lightning.m_pos = m_caster_pos;
	m_lightning.m_beta = fBeta;
	m_lightning.m_alpha = falpha;
	
	m_lightning.m_caster = m_caster;
	m_lightning.m_level = m_level;
	
	m_lightning.Update(timeDelta);
	m_lightning.Render();
	
	ARX_SOUND_RefreshPosition(m_snd_loop, entities[m_caster]->pos);
}



ConfuseSpell::ConfuseSpell()
	: SpellBase()
	, tex_p1(NULL)
	, tex_trail(NULL)
{}

void ConfuseSpell::Launch() {
	
	ARX_SOUND_PlaySFX(SND_SPELL_CONFUSE, &entities[m_target]->pos);
	
	m_hasDuration = true;
	m_fManaCostPerSecond = 1.5f;
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 5000;
	
	
	tex_p1 = TextureContainer::Load("graph/obj3d/textures/(fx)_tsu_blueting");
	tex_trail = TextureContainer::Load("graph/obj3d/textures/(fx)_bandelette_blue");
	
	const char tex[] = "graph/obj3d/interactive/fix_inter/fx_papivolle/fx_papivolle.tea";
	ANIM_HANDLE * anim_papii = EERIE_ANIMMANAGER_Load(tex);
	
	ANIM_Set(au, anim_papii);
	au.next_anim = NULL;
	au.cur_anim = anim_papii;
	au.ctime = 0;
	au.flags = EA_LOOP;
	au.nextflags = 0;
	au.lastframe = 0;
	au.pour = 0;
	au.fr = 0;
	au.altidx_cur = 0;
	au.altidx_next = 0;
	
	m_targets.push_back(m_target);
}

void ConfuseSpell::End() {
	
	m_targets.clear();
	endLightDelayed(m_light, 500);
}

void ConfuseSpell::Update(float timeDelta) {
	
	ARX_UNUSED(timeDelta);
	
	Vec3f pos = entities[m_target]->pos;
	if(m_target != PlayerEntityHandle) {
		pos.y += entities[m_target]->physics.cyl.height - 30.f;
	}
	
	long idx = entities[m_target]->obj->fastaccess.head_group_origin;
	if(idx >= 0) {
		pos = entities[m_target]->obj->vertexlist3[idx].v;
		pos.y -= 50.f;
	}
	
	eCurPos = pos;
	
	RenderMaterial mat;
	mat.setDepthTest(false);
	mat.setBlendType(RenderMaterial::Additive);
	mat.setTexture(tex_trail);
	
	Anglef stiteangle = Anglef(0.f, -glm::degrees(arxtime.get_updated() * ( 1.0f / 500 )), 0.f);
	Draw3DObject(spapi, stiteangle, eCurPos, Vec3f_ONE, Color3f::white, mat);
	
	for(int i = 0; i < 6; i++) {
		
		PARTICLE_DEF * pd = createParticle();
		if(!pd) {
			break;
		}
		
		Vec2f p = glm::diskRand(15.f);
		pd->ov = eCurPos + Vec3f(p.x, 0.f, p.y);
		
		pd->move = Vec3f(0.f, Random::getf(1.f, 4.f), 0.f);
		pd->siz = 0.25f;
		pd->tolive = Random::get(2300, 3300);
		pd->tc = tex_p1;
		pd->special = PARTICLE_GOLDRAIN | FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION
					  | DISSIPATING;
		pd->fparam = 0.0000001f;
		
		Color3f baseColor = Color3f(0.4f, 0.2f, 0.4f);
		Color3f randomFactor = Color3f(0.4f, 0.6f, 0.4f);
		Color3f c = baseColor + randomColor3f() * randomFactor;
		while(glm::abs(c.r - c.g) > 0.3f && glm::abs(c.g - c.b) > 0.3f) {
			c = baseColor + randomColor3f() * randomFactor;
		}
		pd->rgb = c * Color3f(0.8f, 0.8f, 0.8f);
	}
	
	if(!lightHandleIsValid(m_light))
		m_light = GetFreeDynLight();

	if(lightHandleIsValid(m_light)) {
		EERIE_LIGHT * light = lightHandleGet(m_light);
		
		light->intensity = 1.3f;
		light->fallstart = 180.f;
		light->fallend   = 420.f;
		light->rgb = Color3f(0.3f, 0.3f, 0.5f) + Color3f(0.2f, 0.f, 0.2f) * randomColor3f();
		light->pos = eCurPos;
		light->duration = 200;
		light->extras = 0;
	}
}

Vec3f ConfuseSpell::getPosition() {
	return getTargetPosition();
}
