#!/bin/sh
PS4=':${LINENO}+'
set -x
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var PARAMETER_FILE 
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var MOT -sec MOT
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var EMCMOT -sec EMCMOT
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var IO -sec IO
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var EMCIO -sec EMCIO
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var TASK -sec TASK
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var HALUI -sec HAL
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var DISPLAY -sec 
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var NML_FILE -sec 
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var NML_FILE -sec EMC


cd /home/dahua/linuxcnc-2.7.14/configs/sim/axis/vismach/puma


#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var INTRO_GRAPHIC 
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var INTRO_TIME -sec 

linuxcncsvr -ini /home/dahua/linuxcnc-2.7.14/configs/sim/axis/vismach/puma/puma.ini &
#/home/dahua/linuxcnc-2.6/scripts/realtime start
/home/dahua/linuxcnc-2.7.14/scripts/realtime start

halcmd loadusr -Wn iocontrol io -ini /home/dahua/linuxcnc-2.7.14/configs/sim/axis/vismach/puma/puma.ini
halcmd loadusr -Wn halui halui -ini /home/dahua/linuxcnc-2.7.14/configs/sim/axis/vismach/puma/puma.ini
halcmd -i /home/dahua/linuxcnc-2.7.14/configs/sim/axis/vismach/puma/puma.ini -f ./puma_sim_6.hal
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var TWOPASS -sec HAL 
#inivar -tildeexpand -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var 



#inivar -tildeexpand -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var 
#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var HALCMD -sec HAL 

halcmd start



axis -ini /home/dahua/linuxcnc-2.7.14/configs/sim/axis/vismach/puma/puma.ini &
milltask -ini /home/dahua/linuxcnc-2.7.14/configs/sim/axis/vismach/puma/puma.ini



#inivar -ini /home/dahua/linuxcnc-2.6/configs/sim/low_graphics/keystick.ini -var SHUTDOWN -sec HAL

echo "clean_all"


halcmd stop
halcmd unload all
halcmd list comp
echo "finsh"