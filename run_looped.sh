#! /bin/bash

for i in {1..1000} ; do
	if [[ $(($i % 10)) == 0 ]] ; then
		echo "$i"
	fi
./run_obf.sh >> output_file.txt
done
