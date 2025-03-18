#!/usr/bin/env python3
# coding=utf-8

import os
import shutil
import sys
import platform
from zipfile import ZipFile, ZipInfo



current_file_dir = os.path.dirname(__file__)  # 当前文件所在的目录

# https://www.cnblogs.com/zhuminghui/p/11699313.html
# 使用Python解压zip、rar文件
# Python zipfile removes execute permissions from binaries
# https://stackoverflow.com/questions/39296101/python-zipfile-removes-execute-permissions-from-binaries

class MyZipFile(ZipFile):
        def _extract_member(self, member, path=None, pwd=None):
            if not isinstance(member, ZipInfo):
                member = self.getinfo(member)
            path = super()._extract_member(member, path, pwd)

            attr = member.external_attr >> 16
            if attr != 0:
                os.chmod(path, attr)
           
            print(path)
            return path

def create_toolchain():
    '''
    基本格式：zipfile.ZipFile(filename[,mode[,compression[,allowZip64]]])
    mode：可选 r,w,a 代表不同的打开文件的方式；r 只读；w 重写；a 添加
    compression：指出这个 zipfile 用什么压缩方法，默认是 ZIP_STORED，另一种选择是 ZIP_DEFLATED；
    allowZip64：bool型变量，当设置为True时可以创建大于 2G 的 zip 文件，默认值 True；

    '''
    TOOLCHAIN_BIN_DIR  = current_file_dir+'/tc32/bin'
    TOOLCHAIN_ZIP_NAME = 'tc32_win.zip'

    plat = platform.system().lower()
    if plat == 'windows':
        print('windows')
        TOOLCHAIN_ZIP_NAME = 'tc32_win.zip'
    elif plat == 'linux':
        print('linux')
        TOOLCHAIN_ZIP_NAME = 'tc32.zip'
        
    if os.path.exists(TOOLCHAIN_BIN_DIR) == False:
        with MyZipFile(current_file_dir+'/'+TOOLCHAIN_ZIP_NAME) as zfp:
            zfp.extractall(path=current_file_dir)
    copy_utilities()

def copy_utilities():
    try:
        shutil.copy(current_file_dir + '/../../../../adapter/utilities/src/tuya_list.c', current_file_dir + '/../../tuyaos/utilities')
        shutil.copy(current_file_dir + '/../../../../adapter/utilities/src/tuya_queue.c', current_file_dir + '/../../tuyaos/utilities')
        shutil.copy(current_file_dir + '/../../../../adapter/utilities/src/tuya_mem_heap.c', current_file_dir + '/../../tuyaos/utilities')
    except IOError as e:
        print("Unable to copy file. %s" % e)
    except:
        print("Unexpected error:", sys.exc_info())






