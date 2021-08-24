esp_idf="$(pwd)/../esp-idf"
esp_tools="$(pwd)/../gcc"
echo -e " \n ${esp_tools}\n"
export PATH="${esp_idf}/components/esptool_py/esptool:$PATH"
export PATH="${esp_idf}/components/espcoredump:$PATH"
export PATH="${esp_idf}/components/partition_table:$PATH"
export PATH="${esp_idf}/components/app_update:$PATH"
export PATH="${esp_idf}/tools:$PATH"


export PATH="${esp_tools}/tools/xtensa-esp32-elf/esp-2021r1-8.4.0/xtensa-esp32-elf/bin:$PATH"
export PATH="${esp_tools}/tools/esp32ulp-elf/2.28.51-esp-20191205/esp32ulp-elf-binutils/bin:$PATH"
export PATH="${esp_tools}/tools/openocd-esp32/v0.10.0-esp32-20210721/openocd-esp32/bin:$PATH"
export PATH="${esp_tools}/python_env/idf4.4_py3.8_env/bin:$PATH"

#export IDF_PATH=/home/rsd/workspace/esp32project/esp-idf
export IDF_PATH="${esp_idf}"
#make all
echo -e "*******************************************************************************\n $PATH \n *******************************************************************************"
if [ "$1" = "build" ]; then

echo -e " \n idf.py build \n"
#make all
idf.py build

elif [ "$1" = "flash" ]; then

echo -e "\n idf.py  flash\n"
#make flash
idf.py flash -p /dev/ttyUSB0
elif [ "$1" = "clean" ]; then

echo -e  "\n idf.py clean\n"
#make clean
idf.py clean

elif [ "$1" = "monitor" ]; then
echo "make monitor"
make monitor

elif [ "$1" = "menuconfig" ]; then
echo -e  "\n idf.py  menuconfig\n"
#make menuconfig
idf.py menuconig
elif [ "$1" = "erase_flash" ]; then
echo "idf.py  erase_flash"
idf.py erase_flash

else

echo "idf.py clean"
#make clean
#make all
idf.py clean
echo "idf.py bulid"
idf.py build


fi
