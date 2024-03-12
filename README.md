# 用户手册
## 基础命令
bg : 找到最近暂停的后台进程并将其运行
fg :  找到最近后台并将其前台运行
clr : 清屏
cd : 改变工作目录 e.g. cd <directory> 将当前目录改变为directory
dir : 列出目录内容 e.g. dir 显示当前目录内容 dir <directory> 显示指定目录内容
echo : 在屏幕上显示文字,若要显示的字符串有空格，要用双引号括出，且双引号和前后要有空格 e.g. echo 111  echo " hello world " echo $x
exec : 执行外部命令，调用并执行指定命令 e.g. exec <command>
help : 显示用户手册中对于某个指令的帮助 e.g. help <command>
jobs : 显示后台进程
pwd : 输出当前工作目录
set : 设置环境变量 e.g. set x=1
test : test <string1> <parameter> <string2> 比较两个字符串并返回表达式bool值。参数：-eq -ne -gt -lt -ge -le
Time : 显示时间
Umask : 无参数时显示当前mask，有参数时将mask设置为参数

## 脚本与交互
运行时在终端输入./myshell 进入用户交互界面
输入 ./myshell <document>自动读入脚本，脚本执行完毕后进入用户交互界面 e.g. ./myshell test.sh

## 重定向
使用输出重定向时，如果重定向字符是>，则创建输出文件，如果存在则覆盖之；如果重定向字符为>>，也会创建输出文件，如果存在则添加到文件尾。e.g. echo <string> > <file>

## 管道
在“|”的两边分别有两个指令，将前一段指令的输出作为下一段指令的输入，管道两边要加空格。e.g. dir | exec wc

## 命令后台操作
在指令后加&，使指令在后台操作。e.g. ls &
