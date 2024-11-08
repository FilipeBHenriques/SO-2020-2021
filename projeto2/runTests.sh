inputdir=$1
outputdir=$2
numthreads=$3

if [ ! -d "$inputdir" ] 
then 
    echo "Error: $inputdir is not a valid input directory "

elif [ ! -d "$outputdir" ] 
then 
    echo "Error: $outputdir is not a valid output directory "

elif (( $numthreads <= 0 ))
then 
    echo "Error: Numero de threads invalido"
else
    for input in $inputdir/*.txt
        do
        for i in  $(seq 1 $numthreads) 
            do 
                filename=$(basename "$input" .deb)
                echo InputFile = $filename NumThreads = $i
                
                (./tecnicofs  $input $outputdir/$filename-$i.txt $i )|tail -1 
            done
        done
fi