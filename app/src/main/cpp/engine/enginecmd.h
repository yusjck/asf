//
// Created by Jack on 2019/7/22.
//

#ifndef ASF_ENGINECMD_H
#define ASF_ENGINECMD_H

#define ENGCTRL_RUNSCRIPT 1
#define ENGCTRL_ABORT 2
#define ENGCTRL_CLEANUP 3
#define ENGCTRL_SETDISPLAYINFO 4
#define ENGCTRL_ENABLECMDSERVER 5
#define ENGCTRL_EVENTNOTIFY 6
#define ENGCTRL_SETPLUGINDIR 7


#define ENGREPORT_SCRIPTSTATE 10001
#define ENGREPORT_SCRIPTLOG 10002
#define ENGREPORT_CLEANUPFINISHED 10003
#define ENGREPORT_CMDSERVERSTATE 10004

#define CMDSERVER_STARTED 1
#define CMDSERVER_START_FAILED 2
#define CMDSERVER_STOPPED 3

#endif //ASF_ENGINECMD_H
