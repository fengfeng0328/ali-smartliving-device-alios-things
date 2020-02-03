#!/bin/bash

##------default config-----------------------------------------------------------------------------------##

default_type="example"		# 产品类型
default_app="smart_outlet"	# 应用名称
default_board="hf-lpt230"	# 模组型号
default_region=SINGAPORE	# 连云区域
default_env=ONLINE			# 连云环境
default_debug=0				# 调试等级
default_args=""				# 其他参数

##-------------------------------------------------------------------------------------------------------##

# 赋值系统环境变量到变量path_tmp
path_tmp=$PATH
# 导入临时环境变量，即交叉编译工具链位置
export PATH=$path_tmp:$(pwd)/Living_SDK/build/compiler/gcc-arm-none-eabi/Linux64/bin/
# 获取UTC时间，单位绝对秒数
start_time=$(date +%s)

# 判断使用的操作系统是否Linux
if [ "$(uname)" = "Linux" ]; then
    CUR_OS="Linux"
# 判断使用的操作系统是否苹果操作系统
elif [ "$(uname)" = "Darwin" ]; then
    CUR_OS="OSX"
# 判断使用的操作系统是否Windows操作系统
elif [ "$(uname | grep NT)" != "" ]; then
    CUR_OS="Windows"
else
    echo "error: unkonw OS"
    exit 1
fi

# 判断命令行输入的第一个参数是否./build.sh clean
if [[ xx"$1" == xxclean ]]; then
	rm -rf Living_SDK/example
	rm -rf out
	git checkout Living_SDK/example
	exit 0
fi

# 提示输出，使用方法
if [[ xx"$1" == xx--help ]] || [[ xx"$1" == xxhelp ]] || [[ xx"$1" == xx-h ]] || [[ xx"$1" == xx-help ]]; then
	echo "./build.sh $default_type $default_app $default_board $default_region $default_env $default_debug $default_args "
	exit 0
fi

# 产品类型(即应用目录的上层目录)
type=$1
if [ xx"$1" == xx ]; then
	type=$default_type
fi

# 应用目录
app=$2
if [ xx"$2" == xx ]; then
	app=$default_app
fi

# 硬件平台
board=$3
if [ xx"$3" == xx ]; then
	board=$default_board
fi

# 激活中心，大陆版选择MAINLAND，海外选择SINGAPORE
# REGION=MAINLAND or SINGAPORE
if [[ xx$4 == xx ]]; then 
	echo "REGION SET AS MAINLAND"
	REGION=$default_region
else
	REGION=$4
fi

# ENV=ONLINE
if [[ xx$5 == xx ]]; then 
	echo "ENV SET AS ONLINE"
	ENV=$default_env
else
	ENV=$5
fi

# CONFIG_DEBUG=0
if [[ xx$6 == xx ]]; then 
	echo "CONFIG_DEBUG SET AS 0"
	CONFIG_DEBUG=$default_debug
else
	CONFIG_DEBUG=$6
fi

if [[ xx$7 == xx ]]; then 
	echo "ARGS SET AS NULL"
	ARGS=$default_args
else
	ARGS="$7"
fi

function update_golden_product()
{
	gp_type=$1
	gp_app=$2
	git submodule init
	git submodule update --remote Products/$gp_type/$gp_app
	if [ $? -ne 0 ]; then
		echo 'code download or update error!'
		exit 1
	fi
}

# 检查是否飞燕子仓库目录，若是，则检查更新相应目录
if [[ "${type}-${app}" == "Smart_outlet-smart_outlet_meter" ]] || [[ "${type}-${app}" == "Smart_lighting-smart_led_strip" ]] || [[ "${type}-${app}" == "Smart_lighting-smart_led_bulb" ]]; then
	echo 'golden sample product--------------------'
	update_golden_product $type $app
fi

# 检测Products/$type/$app是否是目录 && !检测文件是否是普通文件（既不是目录，也不是设备文件）
if [[ -d Products/$type/$app ]] && [[ ! -f prebuild/lib/board_$board.a ]]; then
	rm -rf Living_SDK/example/$app
	# git checkout Living_SDK/example
fi

echo "----------------------------------------------------------------------------------------------------"
echo "PATH=$PATH"
echo "OS: ${CUR_OS}"
echo "Product type=$type app_name=$app board=$board REGION=$REGION ENV=$ENV CONFIG_DEBUG=$CONFIG_DEBUG ARGS=$ARGS"
echo "----------------------------------------------------------------------------------------------------"
# 检测Products/$type/$app是否是目录 && !检测文件是否是普通文件（既不是目录，也不是设备文件）,这里主要判断是否缺失makefile
if [[ -d Products/$type/$app ]] && [[ ! -f Products/$type/$app/makefile ]]; then
	./tools/cmd/linux64/awk '{ gsub("'"smart_outlet"'","'"$app"'");  print $0 }' Products/Smart_outlet/smart_outlet/makefile > Products/$type/$app/makefile
fi
function build_end()
{
	end_time=$(date +%s)
	cost_time=$[ $end_time-$start_time ]
	echo "build time is $(($cost_time/60))min $(($cost_time%60))s"
	export PATH=$path_tmp
}

function build_sdk()
{
	rm -rf out/$app@/@${board}/*
	if [[ -d Living_SDK/example ]] && [[ -d Products/$type ]] && [[ -d Products/$type/$app ]]; then
		rm -rf Living_SDK/example/$app	# 删除同名文件夹
		cp -rf Products/$type/$app Living_SDK/example/$app	# 复制同名文件夹到对应位置(这写法，不想吐槽)
		
		./tools/cmd/linux64/awk '{ gsub("'./make.settings'","example/${APP_FULL}/make.settings"); gsub("'"\-DAOS_TIMER_SERVICE"'","'" "'"); gsub("'"\?= MAINLAND"'","'"?= ${REGION}"'"); gsub("'"\?= ONLINE"'","'"?= ${ENV}"'"); gsub("'"CONFIG_DEBUG\ \?=\ 0"'","'"CONFIG_DEBUG ?= $CONFIG_DEBUG"'");   print $0 }' Products/$type/$app/$app.mk > Living_SDK/example/${app}/$app.mk
		cd Living_SDK
		aos make clean
		echo -e "aos make $app@${board} CONFIG_SERVER_REGION=${REGION} CONFIG_SERVER_ENV=${ENV} CONFIG_DEBUG=${CONFIG_DEBUG} $ARGS"
		aos make $app@${board} CONFIG_SERVER_REGION=${REGION} CONFIG_SERVER_ENV=${ENV} CONFIG_DEBUG=${CONFIG_DEBUG} "$ARGS"	# 实质还是按照旧方法编译
		cd -
		if [[ -f Living_SDK/out/$app@${board}/binary/$app@${board}.bin ]]; then
			cp -rfa Living_SDK/out/$app@${board}/binary/* out/$app@${board}
			build_end
			exit 0
		else
			echo "build failed!"
			exit 1
		fi
	fi
}

# 创建生成文件目录(-p 递归创建文件夹，若文件夹不存在，则创建)
mkdir -p out;mkdir -p out/$app@${board}
rm -rf out/$app@${board}/*	# 删除缓存文件，良好习惯

# 优先在Products目录下查找相应的产品类型和应用目录，若不存在，则在Living_SDK/example目录下查找应用目录
if [[ ! -d Products/$type ]] || [[ ! -d Products/$type/$app ]]; then
	echo "path of Products/$type or Products/$type/$app don't exist!"
	if [[ ! -d Living_SDK/example/$app ]]; then
		echo "path of Living_SDK/example  don't exist!"
		exit 1
	else
		cd Living_SDK
		aos make clean
		echo -e "aos make $app@${board} CONFIG_SERVER_REGION=${REGION} CONFIG_SERVER_ENV=${ENV} CONFIG_DEBUG=${CONFIG_DEBUG} $ARGS"	
		aos make $app@${board} CONFIG_SERVER_REGION=${REGION} CONFIG_SERVER_ENV=${ENV} CONFIG_DEBUG=${CONFIG_DEBUG} "$ARGS"	# 使用旧版本方法编译
		cd -
		if [[ -f Living_SDK/out/$app@${board}/binary/$app@${board}.bin ]]; then
			cp -rfa Living_SDK/out/$app@${board}/binary/* out/$app@${board}
			build_end
			exit 0
		else
			echo "build failed!"
			exit 1
		fi
	fi
fi

##  build app  ##
if [[ "${board}" == "hf-lpb130" ]] || [[ "${board}" == "hf-lpb135" ]] || [[ "${board}" == "hf-lpt230" ]] || [[ "${board}" == "hf-lpt130" ]] || [[ "${board}" == "uno-91h" ]]; then
	./tools/5981a.sh  $type $app ${board} ${REGION} ${ENV} ${CONFIG_DEBUG} "${ARGS}"
elif [[ "${board}" == "mk3060" ]] || [[ "${board}" == "mk3061" ]]; then
	##  moc108  ##
	./tools/mk3060.sh $type $app ${board} ${REGION} ${ENV} ${CONFIG_DEBUG} "${ARGS}"
elif [[ "${board}" == "mk3080" ]] || [[ "${board}" == "mk3092" ]] || [[ "${board}" == "mk5080" ]]; then
	##  rtl8710bn  ##
	./tools/mk3080.sh  $type $app ${board} ${REGION} ${ENV} ${CONFIG_DEBUG} "${ARGS}"
elif [[ "${board}" == "mx1270" ]]; then
	./tools/mx1270.sh  $type $app ${board} ${REGION} ${ENV} ${CONFIG_DEBUG} "${ARGS}"
elif [[ "${board}" == "asr5501" ]]; then
	./tools/asr5501.sh  $type $app ${board} ${REGION} ${ENV} ${CONFIG_DEBUG} "${ARGS}"
else
	# echo -e "build paras error: you can build board ${board} with follow step: \n cd Living_SDK\n aos make $app@${board} CONFIG_SERVER_REGION=${REGION} CONFIG_SERVER_ENV=${ENV} CONFIG_DEBUG=${CONFIG_DEBUG}"
	# exit 1
	build_sdk
fi

if [[ ! -f out/$app@${board}/$app@${board}.bin ]]; then
	echo "build failed!"
	exit 1
fi

build_end
exit 0


