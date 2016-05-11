#!/bin/bash

EXE="./waf --run parc-acm-icn-2016"

mkdir -p output

prefix=output/`date "+%Y%m%d_%H%M%S"`

link_failure=$prefix-link_failure
add_replicas=$prefix-add_replicas
prefix_delete=$prefix-prefix_delete
command_log=$prefix-command_log

REPS="1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20"
REPLICAS="1 2 3 4 5"

SEED=$RANDOM

for i in $REPS; do
	echo "*** REP $i" | tee -a $command_log
	for r in $REPLICAS; do
		output=${link_failure}_${r}
		echo "$EXE --command-template=\"%s --seed=$SEED --test=link_failure --replicas=$r\" | grep Delta | tee -a $output" | tee -a $command_log
		$EXE --command-template="%s --seed=$SEED --test=link_failure --replicas=$r" | grep "Delta" | tee -a $output
		let "SEED+=1"

		if ( false ); then
			output=${prefix_delete}_${r}
			echo "$EXE --command-template=\"%s --seed=$SEED --test=prefix_delete --replicas=$r\" | grep Delta | tee -a $output" | tee -a $command_log
			$EXE --command-template="%s --seed=$SEED --test=prefix_delete --replicas=$r" | grep "Delta" | tee -a $output
			let "SEED+=1"

			output=${add_replicas}_${r}
			echo "$EXE --command-template=\"%s --seed=$SEED --test=add_replicas --replicas=$r\" | grep Delta | tee -a $output" | tee -a $command_log
			$EXE --command-template="%s --seed=$SEED --test=add_replicas --replicas=$r" | grep "Delta" | tee -a $output
			let "SEED+=1"
		fi

	done
done


for r in $REPLICAS; do
	src/ccns3Examples/examples/parc-acm-icn-2016.awk ${link_failure}-${r} >> ${link_failure.dat}
done

