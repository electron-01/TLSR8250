#!/usr/bin/env python3
# coding=utf-8

import os
import sys
import pip
import urllib.request

# 检查 git 是否安装并进行安装
def check_and_install(KEY,MODULE):
    if not KEY in sys.modules.keys():
        print('install',MODULE)
        pip.main(['install', MODULE])  
try:
    from git import Repo
except:
    check_and_install('git','GitPython')
    from git import Repo

current_file_dir = os.path.dirname(__file__)  # 当前文件所在的目录

def get_abspath(relative_path):
    if os.path.isabs(relative_path):
        return os.path.abspath(relative_path).replace('\\','/')
    else:
        return os.path.abspath(os.getcwd()+'/'+relative_path).replace('\\','/')

def get_ide_tool():
    IDE_TOOL_LINK = 'https://gitee.com/work_tuya/all_in_one_ide_tool'
    try:
        myURL = urllib.request.urlopen(url=IDE_TOOL_LINK, timeout=5)
        if myURL.getcode() == 200:
            print('clone ide_tool from gitee……')
            Repo.clone_from(IDE_TOOL_LINK, IDE_TOOL_DIR, branch='V2.X')
    except Exception as e:
        # gitee 超时
        IDE_TOOL_LINK = 'https://github.com/tuya/tuya_os_compile_script_on_windows.git'
        myURL = urllib.request.urlopen(url=IDE_TOOL_LINK)
        if myURL.getcode() == 200:
            print('clone ide_tool from github……')
            Repo.clone_from(IDE_TOOL_LINK, IDE_TOOL_DIR, branch='V2.X')
        else:
            print("Can not access github and gitee, please check your network!")


# 1-检查vendor本身工具链
sys.path.append(current_file_dir+'/toolchain/software')
from create_toolchain import create_toolchain

create_toolchain()


# 2- 下载通用 IDE 工具
PROJECT_ROOT_DIR = get_abspath(current_file_dir + '/../..')
IDE_TOOL_DIR     = PROJECT_ROOT_DIR+'/.ide_tool'
if os.path.exists(IDE_TOOL_DIR) == False:
    get_ide_tool()
    
sys.path.append(IDE_TOOL_DIR)
from ide_tool import ide_tool_front,ide_tool_back

# 3-构建编译
BUILD_COMMAND = ''
IDE_KIND = 'gcc'
PARAMS_NUM = len(sys.argv)-1
if PARAMS_NUM > 0:
    BUILD_COMMAND = sys.argv[1]
if PARAMS_NUM == 2:
    IDE_KIND = sys.argv[2]

DEMO_PATH = './samples/ble_peripheral_sample'
DEMO_NAME = 'ble_peripheral_sample'
DEMO_FIRMWARE_VERSION = '0.3.6'
DEMO_OUTPUT_PATH = './_output'
BOARD_NAME = 'tlsr825x_ble'

if BUILD_COMMAND == 'pr-build':
    if PARAMS_NUM == 6:
        DEMO_PATH = sys.argv[2];
        BOARD_NAME = sys.argv[3];
        DEMO_OUTPUT_PATH = sys.argv[4];
        DEMO_NAME = sys.argv[5];
        DEMO_FIRMWARE_VERSION = sys.argv[6];
    ide_tool_front(PROJECT_ROOT_DIR,DEMO_PATH,BOARD_NAME,DEMO_OUTPUT_PATH,DEMO_NAME,DEMO_FIRMWARE_VERSION)
elif BUILD_COMMAND == 'build':
    ide_tool_back('build','./project.json',IDE_KIND)
elif BUILD_COMMAND == 'sdk':
    ide_tool_back('sdk','./project.json',IDE_KIND)
elif BUILD_COMMAND == 'flash_user':
    ide_tool_back('flash_user','./project.json',IDE_KIND)
elif BUILD_COMMAND == 'flash_all':
    ide_tool_back('flash_all','./project.json',IDE_KIND)
elif BUILD_COMMAND == 'erase':
    print('erase\n')





