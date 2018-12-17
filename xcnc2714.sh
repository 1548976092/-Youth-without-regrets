#!/bin/bash
PS4=':${LINENO}+'
set -x
cd /home/dahua/linuxcnc-2.6/configs/sim/axis/vismach/puma


linuxcncsvr -ini /home/dahua/linuxcnc-2.6/configs/sim/axis/vismach/puma/puma.ini &


/home/dahua/linuxcnc-2.6/scripts/realtime start


halcmd loadusr -Wn iocontrol io -ini /home/dahua/linuxcnc-2.6/configs/sim/axis/vismach/puma/puma.ini
halcmd loadusr -Wn halui halui -ini /home/dahua/linuxcnc-2.6/configs/sim/axis/vismach/puma/puma.ini

halcmd -i /home/dahua/linuxcnc-2.6/configs/sim/axis/vismach/puma/puma.ini -f puma_sim_6.hal


halcmd start

axis -ini /home/dahua/linuxcnc-2.6/configs/sim/axis/vismach/puma/puma.ini &
milltask -ini /home/dahua/linuxcnc-2.6/configs/sim/axis/vismach/puma/puma.ini 





