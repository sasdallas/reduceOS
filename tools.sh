echo Updating apt...
sudo apt-get update

echo Downloading build tools...

sudo apt-get install -y binutils gcc nasm python3 perl qemu qemu-system grub2-common

echo Install i686-elf toolchain...
echo "nevermind we gonna skip that, doesn't work"
#wget https://github.com/lordmilko/i686-elf-tools/releases/download/7.1.0/i686-elf-tools-linux.zip
#unzip i686-elf-tools-linux.zip
echo Complete! Ready for build.