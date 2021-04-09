/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "engine/gamedef.h"
#include "engine/game_env.h"
#include "engine/rule_actor.h"

#include "engine/cmd.gen.h"
#include "engine/cmd_specific.gen.h"
#include "cmd_specific.gen.h"
#include "engine/ai.h"
#include "rule_ai.h"

int GameDef::GetNumUnitType() {
    return NUM_MINIRTS_UNITTYPE;
}

int GameDef::GetNumAction() {
    return NUM_AISTATE;     //13
}

bool GameDef::IsUnitTypeBuilding(UnitType t) const{
    return (t == BASE) || (t == RESOURCE) || (t == BARRACKS);
}

bool GameDef::HasBase() const{ return true; }

bool GameDef::CheckAddUnit(RTSMap *_map, UnitType, const PointF& p) const{
    return _map->CanPass(p, INVALID);
}

void GameDef::GlobalInit() {
    reg_engine();
    reg_engine_specific();
    reg_minirts_specific();

    // InitAI.
    AIFactory<AI>::RegisterAI("simple", [](const std::string &spec) {
        (void)spec;
        AIOptions ai_options;
        return new SimpleAI(ai_options);
    });

    AIFactory<AI>::RegisterAI("hit_and_run", [](const std::string &spec) {
        (void)spec;
        AIOptions ai_options;
        return new HitAndRunAI(ai_options);
    });
}


void GameDef::Init() {
    _units.assign(GetNumUnitType(), UnitTemplate());
    _units[RESOURCE] = _C(0, 1000, 1000, 0, 0, 0, 0, vector<int>{0, 0, 0, 0}, vector<CmdType>{}, ATTR_INVULNERABLE);
    _units[WORKER] = _C(5, 50, 0, 0.03, 2, 1, 3, vector<int>{0, 0, 0, 0}, vector<CmdType>{ATTACK,MOVE,GATHER,BUILD});
    _units[MELEE_ATTACKER] = _C(100, 100, 0, 0.01, 100, 1, 2, vector<int>{0, 500,0, 0}, vector<CmdType>{MOVE,ATTACK});
    _units[RANGE_ATTACKER] = _C(100, 100, 100, 0, 100, 1, 1, vector<int>{0, 600, 0, 0}, vector<CmdType>{ATTACK});
    // 炮弹伤害
    _units[BARRACKS] = _C(0, 99, 1, 0, 100, 0, 0, vector<int>{0, 0, 0, 0}, vector<CmdType>{BUILD,MOVE});
    _units[BASE] = _C(500, 150, 0, 0, 100, 1, 5, {0, 0, 0, 2}, vector<CmdType>{BUILD,ATTACK});
}

vector<pair<CmdBPtr, int> > GameDef::GetInitCmds(const RTSGameOptions&) const{
      vector<pair<CmdBPtr, int> > init_cmds;
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateMap(INVALID, 0, 200)), 1));
      init_cmds.push_back(make_pair(CmdBPtr(new CmdGenerateUnit(INVALID)), 2));
      return init_cmds;
}

PlayerId GameDef::CheckWinner(const GameEnv& env, bool /*exceeds_max_tick*/) const {
    return env.CheckWinner(BASE,env);
    // return env.CheckBase(BASE);
}

// 移除死亡单位
void GameDef::CmdOnDeadUnitImpl(GameEnv* env, CmdReceiver* receiver, UnitId /*_id*/, UnitId _target) const{
    Unit *target = env->GetUnit(_target);
    if (target == nullptr) return;
    receiver->SendCmd(CmdIPtr(new CmdRemove(_target)));
}
