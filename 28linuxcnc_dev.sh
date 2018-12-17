#!/bin/bash
PS4='${LINENO} ~ '
set -x



export INI_FILE_NAME=/home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma/puma.ini

cd /home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma


linuxcncsvr -ini /home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma/puma.ini


/home/dahua/linuxcnc-dev/scripts/realtime start

halcmd loadusr -Wn iocontrol io -ini /home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma/puma.ini
halcmd loadusr -Wn halui halui -ini /home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma/puma.ini
halcmd -i /home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma/puma.ini -f ./puma_sim_6.hal

halcmd loadusr -Wn inihal milltask -ini /home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma/puma.ini  &
halcmd start

axis -ini /home/dahua/linuxcnc-dev/configs/sim/axis/vismach/puma/puma.ini  
Cleanup

exit $result

