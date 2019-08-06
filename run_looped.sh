#! /bin/bash
printf "0        10        20        30        40        50        60        70        80        90        100"

for i in {0..1000} ; do
	if [[ $(($i % 10)) == 0 ]] ; then
		printf "."
	else
		printf "."
	fi	
	if [[ $(($i % 100)) == 0 ]] ; then 
		printf "\n"
	fi
	#cp hello_world hello_world_copy
	#./run_obf.sh >> looped.log
	#mv hello_world_mal hello_world
done
