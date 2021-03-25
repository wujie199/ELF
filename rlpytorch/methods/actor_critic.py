# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

import torch
import torch.nn as nn
from torch.autograd import Variable
import math

from ..args_provider import ArgsProvider
from .policy_gradient import PolicyGradient
from .discounted_reward import DiscountedReward
from .value_matcher import ValueMatcher
from .utils import add_err

# Actor critic model.
class ActorCritic:
    ''' An actor critic model '''
    def __init__(self):
        ''' Initialization of `PolicyGradient`, `DiscountedReward` and `ValueMatcher`.
        Initialize the arguments needed (num_games, batchsize, value_node) and in child_providers.
        '''
        self.pg = PolicyGradient()
        self.discounted_reward = DiscountedReward()
        self.value_matcher = ValueMatcher()

        self.args = ArgsProvider(
            call_from = self,
            define_args = [
            ],
            more_args = ["num_games", "batchsize", "value_node"],
            child_providers = [ self.pg.args, self.discounted_reward.args, self.value_matcher.args ],
        )

    def update(self, mi, batch, stats):
        ''' 
        需要的参数:
            mi(`ModelInterface`): mode interface used
            batch(dict): batch of data. Keys in a batch:
                ``s``: state,
                ``r``: immediate reward,
                ``terminal``: if game is terminated
            stats(`Stats`): Feed stats for later summarization.
        '''
        m = mi["model"]
        args = self.args
        value_node = self.args.value_node

        T = batch["s"].size(0)

        state_curr = m(batch.hist(T - 1))
        # 设置初始的 Reward
        self.discounted_reward.setR(state_curr[value_node].squeeze().data, stats)

        err = None
        
        for t in range(T - 2, -1, -1):
            bht = batch.hist(t)
            # 先执行一次，获得当前状态下的 权重矩阵 和行为
            state_curr = m.forward(bht)

            V = state_curr[value_node].squeeze()

            # 计算 reward
            R = self.discounted_reward.feed(
                dict(r=batch["r"][t], terminal=batch["terminal"][t]),
                stats=stats)

            # 计算权重矩阵的误差
            policy_err = self.pg.feed(R-V.data, state_curr, bht, stats, old_pi_s=bht)
            err = add_err(err, policy_err)
            # 计算价值 V 的误差
            err = add_err(err, self.value_matcher.feed({ value_node: V, "target" : R}, stats))

        stats["cost"].feed(err.data[0] / (T - 1))
        # 反向传播
        err.backward()
