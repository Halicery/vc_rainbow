gcc -c bmp.c
gcc -c lzw.c
gcc -c inflate.c
gcc -c idct.c
gcc -c -I..\GETC jpeg81.c
gcc -c -I..\GETC gif.c
gcc -c -I..\GETC -Wno-multichar png.c
gcc -c -I..\GETC -Wno-multichar tiff.c
gcc -c -I..\GETC unzip.c
