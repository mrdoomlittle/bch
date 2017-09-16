C_LFLAGS="-L../bci/lib -L../bci/8xdrm/lib"
C_IFLAGS="-I../bci/inc -I../bci/8xdrm/inc"
cd ../bci;
sh compile.sh
cd ../bch;
gcc $C_IFLAGS $C_LFLAGS -std=gnu11 -o bch bch.c -lbci -lm -l8xdrm
