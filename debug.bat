@echo off
chcp 65001 > nul
set /p input=请输入调用堆栈(地址)：
cd code/cm_backtrace
call addr2line -e "../../GNU ARM v12.2.1 - Default/P3A.out" -f %input%
pause