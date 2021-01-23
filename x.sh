#find ./ -iname "*.o" -delete -print
#set -v
#rm sz81 sz81_2.1.7_mipsel-linux-uclibc.tar.gz
#rm *~

export PATH="${PATH}:/opt/mipsel-linux-uclibc/usr/bin/"
make -f Makefile.dingux-A320-dev-linux-i386
./sz81
#echo "killall sz81" | nc 10.1.0.2
#ncftpput 10.1.0.2 /local/emulators/dingux-sz81 sz81
