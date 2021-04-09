MODEL=$1
FRAMESKIP=$2
NUM_EVAL=500

game=./rts/game_MC/game model_file=./rts/game_MC/model model=actor_critic python3 eval.py --save_replay_prefix ./res --num_games 64 --batchsize 1 --tqdm --load save-221.bin --gpu 0 --players "fs=20,type=AI_NN;fs=20,type=AI_SIMPLE" --eval_stats winrate --num_eval $NUM_EVAL --additional_labels id,last_terminal,seq --shuffle_player --num_frames_in_state 1 --greedy --keys_in_reply V #--omit_keys Wt,Wt2,Wt3 #
