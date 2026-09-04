from __future__ import annotations
from pathlib import Path
from typing import TYPE_CHECKING

import logging

from sb3_contrib import MaskablePPO
from sb3_contrib.common.maskable.policies import MaskableMultiInputActorCriticPolicy
from sb3_contrib.common.maskable.utils import get_action_masks
from stable_baselines3.common.utils import set_random_seed


from qiskit import QuantumCircuit
from qiskit.transpiler import Target

from reward import Reward
from env import MQSSPredictorEnv

mqss_predictor_logger = logging.getLogger("mqss-predictor")
mqss_predictor_logger.setLevel(logging.INFO)

class MQSSDevPassPredictor:
    def __init__(self,
            reward: Reward,
            device: Target,
            path_train_circuits: Path | None = None):
        
        self.env = MQSSPredictorEnv(
            reward_function=reward, device=device, path_training_circuits=path_train_circuits
        )
        self.device_name = device.description
        self.reward = reward

    def train_model(
            self,
            timesteps: int = 1000,
            model_name: str = "mqss_scheduler_model",
            verbose: int = 2,
            test: bool = False
    ):
        if test:
            set_random_seed(42)
            n_steps = 16
            n_epochs = 1
            batch_size = 8
            progress_bar = False
        else:
            n_steps = 1024
            n_epochs = 16
            batch_size = 64
            progress_bar = True
        
        mqss_predictor_logger.info(f"Start training MQSSDevPassPredictor on device: {self.device_name}")
        # model = MaskablePPO(
        #     MaskableMultiInputActorCriticPolicy,
        #     self.env,
        #     verbose=verbose,
        #     tensorboard_log="./" + model_name + "_tensorboard",
        #     gamma=0.98,
        #     n_steps=n_steps,
        #     batch_size=batch_size,
        #     n_epochs=n_epochs,
        # )
        # model.learn(total_timesteps=timesteps, progress_bar=progress_bar)
        # model.save("./" + model_name + "_" + self.reward + "_" + self.device_name)
