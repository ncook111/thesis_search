#!/bin/bash

# ######## Dataset_1 ########
# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c ",Male," bench_data/all_input_data/csv/large_data/data_1/1_large.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_1 Large Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c ",Male," bench_data/all_input_data/csv/medium_data/data_1/1_medium.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_1 Medium Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c ",Male," bench_data/all_input_data/csv/small_data/data_1/1_small.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_1 Small Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# ######## Dataset_2 ########

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c "1994-01-01," bench_data/all_input_data/csv/large_data/data_2/2_large.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_2 Large Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c "1994-01-01," bench_data/all_input_data/csv/medium_data/data_2/2_medium.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_2 Medium Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c "1994-01-01," bench_data/all_input_data/csv/small_data/data_2/2_small.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_2 Small Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# ######## Dataset_3 ########

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c "Pavel" bench_data/all_input_data/csv/large_data/data_3/3_large.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_3 Large Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c "Pavel" bench_data/all_input_data/csv/medium_data/data_3/3_medium.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_3 Medium Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c "Pavel" bench_data/all_input_data/csv/small_data/data_3/3_small.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_3 Small Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

######## Dataset_4 ########

# for i in $(seq 1 15); do
#   { gtime -f "%e" grep -c "2025-09-30 " bench_data/all_input_data/csv/large_data/data_4/4_large_1.csv bench_data/all_input_data/csv/large_data/data_4/4_large_2.csv bench_data/all_input_data/csv/large_data/data_4/4_large_3.csv bench_data/all_input_data/csv/large_data/data_4/4_large_4.csv > /dev/null; } 2>> times.txt
# done
# awk '{sum+=$1; count++} END {print "Data_4 Large Average time:", sum/count, "seconds"}' times.txt
# rm times.txt

for i in $(seq 1 15); do
  { gtime -f "%e" grep -c "2025-09-30 " bench_data/all_input_data/csv/medium_data/data_4/4_medium_1.csv bench_data/all_input_data/csv/medium_data/data_4/4_medium_2.csv bench_data/all_input_data/csv/medium_data/data_4/4_medium_3.csv bench_data/all_input_data/csv/medium_data/data_4/4_medium_4.csv > /dev/null; } 2>> times.txt
done
awk '{sum+=$1; count++} END {print "Data_4 Medium Average time:", sum/count, "seconds"}' times.txt
rm times.txt

for i in $(seq 1 15); do
  { gtime -f "%e" grep -c "2025-09-30 " bench_data/all_input_data/csv/smalldata/data_4/4_small_1.csv bench_data/all_input_data/csv/small_data/data_4/4_small_2.csv bench_data/all_input_data/csv/small_data/data_4/4_small_3.csv bench_data/all_input_data/csv/small_data/data_4/4_small_4.csv > /dev/null; } 2>> times.txt
done
awk '{sum+=$1; count++} END {print "Data_4 Small Average time:", sum/count, "seconds"}' times.txt
rm times.txt