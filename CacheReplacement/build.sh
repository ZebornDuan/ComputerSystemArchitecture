policy=(0 1 2)
for file in `ls traces/*.out.trace.gz`
do
	for p in ${policy[@]}
	do
		echo `time -p bin/CMPsim.usetrace.32 -threads 1 -t $file -o runs/${file:7:${#file}-20}$p.stats -cache UL3:1024:64:16 -LLCrepl $p`
	done
done
