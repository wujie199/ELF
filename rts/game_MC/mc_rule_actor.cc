/**
* Copyright (c) 2017-present, Facebook, Inc.
* All rights reserved.

* This source code is licensed under the BSD-style license found in the
* LICENSE file in the root directory of this source tree.
*/
#include<unistd.h>
#include "mc_rule_actor.h"
#include "game_action.h"
bool MCRuleActor::ActByState2(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
    assigned_cmds->clear();
    *state_string = "";

    RegionHist hist;

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        act_per_unit(env, u, &state[0], &hist, state_string, assigned_cmds);
    }

    return true;
}

bool MCRuleActor::ActByState(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {
    // Each unit can only have one command. So we have this map.
    // cout << "Enter ActByState" << endl << flush;
    assigned_cmds->clear();
    *state_string = "NOOP";

    // 建造飞机
    if (state[STATE_BUILD_WORKER]) {
       *state_string = "Build worker..NOOP";
       const Unit *base = _preload.Base();
       if (IsIdle(*_receiver, *base)) {
           if (_preload.BuildIfAffordable(WORKER)) {
               *state_string = "Build worker..Success";
               store_cmd(base, _B(WORKER), assigned_cmds);
           }
       }
    }


    const auto& my_troops = _preload.MyTroops();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    // const auto& all_my_troops = _preload.AllMyTroops();
    // const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();
    // std::cout <<  "-----------enemy-------: " << enemy_troops_in_range.empty() <<std::endl;
    // std::cout <<  "-----------enemy-------: " << enemy_troops_in_range.empty() <<  "-----------enemy[0]-------: " << enemy_troops_in_range[0] <<   "-----------enemy[0].id-------: "<< enemy_troops_in_range[0]->GetId() << std::endl;    // 1 是空   0 非空


    // 飞机前进，投弹
    if (state[STATE_BUILD_BARRACK]) {
        // 前进
       if (_preload.Affordable(BARRACKS)) {
           const Unit *u = GameEnv::PickFirstIdle(my_troops[WORKER], *_receiver);
           if (u != nullptr) {
               CmdBPtr cmd = _preload.GetMOVECmd();
               if (cmd != nullptr) {
                   *state_string = "Forward..Success";
                   store_cmd(u, std::move(cmd), assigned_cmds);
               }
           }
        }
        // 建造兵营
        for(const Unit *u:my_troops[WORKER]){
            // const Unit *u = GameEnv::PickFirst(my_troops[WORKER], *_receiver, GATHER);
                if(PointF::L2Sqr(u->GetPointF(),_preload.GetEnemyBaseLoc()) < 120.0f){
                        CmdBPtr cmd1 = _preload.GetBuildBarracksCmd1(env,u->GetPointF());
                            if (cmd1 != nullptr) {
                                store_cmd(u, std::move(cmd1), assigned_cmds);
                    }
            }
        }
    }

    // 建造炮弹
    if(state[STATE_BUILD_MELEE_TROOP]){
        if (_preload.Affordable(MELEE_ATTACKER)) {
            const Unit *u = GameEnv::PickFirstIdle(my_troops[BARRACKS], *_receiver);
            if (u != nullptr) {
                *state_string = "Build Bullet..Success";
                store_cmd(u, _B(MELEE_ATTACKER), assigned_cmds);
                _preload.Build(MELEE_ATTACKER);
            }
        }
    }

        // 飞机返回
    if(state[STATE_START]){
        for(const Unit *u : my_troops[WORKER]){
           if (u != nullptr) {
               CmdBPtr cmd = _preload.GetReturn();
               if (cmd != nullptr) {
                   *state_string = "Return..Success";
                   store_cmd(u, std::move(cmd), assigned_cmds);
               }
           }
        }
    }

    // 炮弹移向目标
    if(state[STATE_DEFEND]){
        *state_string = "Bullet..forward";
        auto cmd = _preload.GetAttackEnemyBaseCmd();
        batch_store_cmds(my_troops[MELEE_ATTACKER], cmd, false, assigned_cmds);
    }

    // 消灭基地附近的炮弹
    if(state[STATE_HIT_AND_RUN]){
        if(my_troops[MELEE_ATTACKER].size() > 0){
            batch_store_cmds(enemy_troops[RANGE_ATTACKER], _A(my_troops[MELEE_ATTACKER][0]->GetId()), false, assigned_cmds);
        }
    }
    
    return true;
}

bool MCRuleActor::GetActSimpleState(vector<int>* state) {
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();

    if(my_troops[WORKER].size() < 4){
        _state[STATE_BUILD_WORKER] = 1;
    }

    // 兵营以1为界限
    if(my_troops[BARRACKS].size() < 1){
        // 飞机前进，并且建造兵营
        _state[STATE_BUILD_BARRACK] = 1;
    }else{
        // 飞机返回
        _state[STATE_START] = 1;
    }

    if(my_troops[BARRACKS].size() > 0 && my_troops[MELEE_ATTACKER].size() < 6){
        // 建造炮弹
        _state[STATE_BUILD_MELEE_TROOP] = 1;
        // 炮弹移向目标
        _state[STATE_DEFEND] = 1;
    }else{
        // 炮弹向目标移动
        _state[STATE_DEFEND] = 1;
    }
    return true;
}


bool MCRuleActor::ActByState3(const GameEnv &env, const vector<int>& state, string *state_string, AssignedCmds *assigned_cmds) {

    // 获取我方所有军队
    const auto& my_troops = _preload.MyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();


    if (state[STATE_BUILD_WORKER]) {
    int i = 0;
    if(!enemy_troops_in_range.empty()){                         // 1 是空   0 非空
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }

    if (state[STATE_BUILD_BARRACK]) {
    int i = 1;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    
    if (state[STATE_BUILD_MELEE_TROOP]) {
    int i = 2;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    
    if (state[STATE_BUILD_RANGE_TROOP]) {
    int i = 3;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[STATE_ATTACK]) {
    int i = 4;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[STATE_ATTACK_IN_RANGE]) {
    int i = 5;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[STATE_HIT_AND_RUN]) {
    int i = 6;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[STATE_DEFEND]) {
    int i = 7;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[NUM_AISTATE]) {
    int i = 8;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[STATE_10]) {
    int i = 9;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[STATE_11]) {
    int i = 10;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
        if (state[STATE_12]) {
    int i = 11;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
        if (state[STATE_13]) {
    int i = 12;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    if (state[NUM_AISTATE]) {
    int i = 13;
    if(!enemy_troops_in_range.empty()){
        auto it = enemy_troops_in_range.begin();
        auto cmd = _A((*it)->GetId());
        batch_store_cmds2(my_troops[RANGE_ATTACKER],cmd,false,assigned_cmds,i);
        }
    }
    return true;
}



bool MCRuleActor::GetActHitAndRunState(vector<int>* state) {
    vector<int> &_state = *state;

    const auto& my_troops = _preload.MyTroops();
    const auto& cnt_under_construction = _preload.CntUnderConstruction();
    const auto& enemy_troops = _preload.EnemyTroops();
    const auto& enemy_troops_in_range = _preload.EnemyTroopsInRange();
    const auto& enemy_attacking_economy = _preload.EnemyAttackingEconomy();

    if (my_troops[WORKER].size() < 3 && _preload.Affordable(WORKER)) {
        _state[STATE_BUILD_WORKER] = 1;
    }
    if (my_troops[WORKER].size() >= 3 && my_troops[BARRACKS].size() + cnt_under_construction[BARRACKS] < 1 && _preload.Affordable(BARRACKS)) {
        _state[STATE_BUILD_BARRACK] = 1;
    }
    if (my_troops[BARRACKS].size() >= 1 && _preload.Affordable(RANGE_ATTACKER)) {
        _state[STATE_BUILD_RANGE_TROOP] = 1;
    }
    int range_troop_size = my_troops[RANGE_ATTACKER].size();
    if (range_troop_size >= 2) {
        if (enemy_troops[MELEE_ATTACKER].empty() && enemy_troops[RANGE_ATTACKER].empty()
          && enemy_troops[WORKER].empty()) {
            _state[STATE_ATTACK] = 1;
        } else {
            _state[STATE_HIT_AND_RUN] = 1;
        }
    }
    if (! enemy_troops_in_range.empty() || ! enemy_attacking_economy.empty()) {
        _state[STATE_DEFEND] = 1;
    }
    return true;
}

bool MCRuleActor::ActWithMap(const GameEnv &env, const vector<vector<vector<int>>>& action_map, string *state_string, AssignedCmds *assigned_cmds) {
    assigned_cmds->clear();
    *state_string = "";

    vector<vector<RegionHist>> hist(action_map.size());
    for (size_t i = 0; i < hist.size(); ++i) {
        hist[i].resize(action_map[i].size());
    }

    const int x_size = env.GetMap().GetXSize();
    const int y_size = env.GetMap().GetYSize();
    const int rx = action_map.size();
    const int ry = action_map[0].size();

    // Then loop over all my troops to run.
    const auto& all_my_troops = _preload.AllMyTroops();
    for (const Unit *u : all_my_troops) {
        // Get the bin id.
        const PointF& p = u->GetPointF();
        int x = static_cast<int>(std::round(p.x / x_size * rx));
        int y = static_cast<int>(std::round(p.y / y_size * ry));
        // [REGION_MAX_RANGE_X][REGION_MAX_RANGE_Y][REGION_RANGE_CHANNEL]
        act_per_unit(env, u, &action_map[x][y][0], &hist[x][y], state_string, assigned_cmds);
    }

    return true;
}
