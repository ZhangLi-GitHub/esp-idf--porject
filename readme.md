## esp-idf 的环境配置样例

​	ESP-idf 目前版本 v4.4-dev-2594-ga20df743f1

clone  **ESP-idf**  下来后可以使用 命令，注意是 ESP-idf 并非本工程， ESP-IDF 的版本也已编译至固件中，因此您也可以使用宏 **IDF_VER** 查看 ESP-IDF 的版本

```c++
git describe --tags --dirty
```

 ### 结构说明

- esp-idf ： esp32 固件,本工程已经取消与原远程仓库的连接 所以无法查看相关版本，如需要其他版本可自行去官网 克隆。本工程是2021年8月克隆的master分支 版本是  v4.4-dev-2594-ga20df743f1
- gcc：  编译工具链存放处，将其纳入版本管理
-  hello_world ：  项目工程

工程结构：

├── CMakeLists.txt
├── example_test.py
├── go.sh
├── main
│   ├── CMakeLists.txt
│   ├── component.mk
│   └── hello_world_main.c
├── Makefile
├── README.md
├── sdkconfig
└── sdkconfig.ci

go.sh是自己写的脚本，方便编译 下载与监视

```c++
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
                                                               
```

## 使用方法

编译：`./go.sh build` 				 清除 ： `./go.sh clean` 
下载：`./go.sh flash`				监视器：.`/go.sh monitor`

- **我们保存这样的结构 开发方便  只需要将工程 放到 hello_world同级，然后将go.sh 复制到工程目录中可直接运行 编译。**
