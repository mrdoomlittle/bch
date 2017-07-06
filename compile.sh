cd ../bci;
sh compile.sh
cd ../bch;
gcc -I../bci/inc -L../bci/lib -std=c11 -o bch bch.c -lbci -lm
