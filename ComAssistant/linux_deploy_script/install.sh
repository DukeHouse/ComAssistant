#!/usr/bin/env bash
linuxType=$(cat /etc/os-release)
ubuntuStr="Ubuntu"
centosStr="CentOS"
deepinStr="Deepin"

chmod +x ComAssistant
echo -e "\033[33m add current path to enviroment variable. \033[0m"
echo "export PATH=$(pwd):$PATH" >> ~/.bashrc
source ~/.bashrc

echo -e "\033[33m add current user to dialout group to access ttys device. \033[0m"
# IS Ubuntu
if [[ $linuxType == *$ubuntuStr* ]]
then

echo -e "\033[33m current linux is Ubuntu. \033[0m"
sudo usermod -a -G dialout $USER

# IS CentOS
elif [[ $linuxType == *$centosStr* ]]
then

echo -e "\033[33m current linux is CentOS. \033[0m"
# check is root account or not
isRoot="root"
if [[ $(echo `whoami`) != $isRoot ]]
then
echo -e "\033[31m Sorry! Please execute the script by root account \033[0m"
exit 1
fi

usermod -a -G dialout $USER

# IS Deepin
elif  [[ $linuxType == *$deepinStr* ]]
then

echo -e "\033[33m current linux is Deepin. \033[0m"
sudo usermod -a -G dialout $USER

else

# untested system
echo -e "\033[33m Untested system, please install ComAssistant by manual. \033[0m"
exit 1

fi

# final output
echo -e "\033[32m ----------------------------------------------------- \033[0m"
echo -e "\033[32m                   CONGRATULATIONS!  \033[0m"
echo -e "\033[32m Next RESTART your PC for the settings to take effect. \033[0m"
echo -e "\033[32m And then you can run ComAssistant \033[0m"
echo -e "\033[32m ----------------------------------------------------- \033[0m"
exit 0
