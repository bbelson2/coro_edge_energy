This data sdet was created on a Raspberry Pi 3 B.
Linux raspberrypi 6.1.19-v7+ #1637 SMP Tue Mar 14 11:04:52 GMT 2023 armv7l GNU/Linux
Raspberry Pi 3 Model B Plus Rev 1.3

conda activate jls2

The following command created this data set. It was run repeatedly until successful; note that the -b option provides backfill, so successful test were not repeated.

python .\run_infer7.py -t 2 -s 10 50 10  -n 10 100 10  -d 128 2048 64 -r 30 -i 10.0.0.113 -o data27 -b

experiments.csv and spikes.csv were copied from data27 to here.

