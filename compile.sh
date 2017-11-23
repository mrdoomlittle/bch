C_LFLAGS="-L../bci/lib -L../bci/bitct/lib"
C_IFLAGS="-I../bci/inc -I../bci/bitct/inc"
cd ../bci;
sh compile.sh
cd ../bch;
gcc $C_IFLAGS $C_LFLAGS -std=gnu11 -o bch bch.c -lmdl-bci -lm -lmdl-bitct
