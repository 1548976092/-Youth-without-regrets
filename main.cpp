/********************************************************************
* Description:   xemc.cc
*   X GUI for the EMC
*
*   Derived from a work by Fred Proctor & Will Shackleford
*   Brought forward and adapted to emc2 by Alex Joni
*
* Author: 
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change:
********************************************************************/

#define __STDC_FORMAT_MACROS

#include "mainwindow.h"
#include <QApplication>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <values.h>             // DBL_MAX, maybe
#include <limits.h>             // DBL_MAX, maybe
#include <stdarg.h>
#include <sys/stat.h>           // struct stat, stat()
#include <unistd.h>
#include <fcntl.h>              // O_CREAT
#include <inttypes.h>

#include "rcs.hh"               // etime()
#include "emc.hh"               // EMC NML
#include "emc_nml.hh"
#include "emcglb.h"             // EMC_NMLFILE, TRAJ_MAX_VELOCITY, TOOL_TABLE_FILE
#include "emccfg.h"             // DEFAULT_TRAJ_MAX_VELOCITY
#include "inifile.hh"           // INIFILE
#include "rcs_print.hh"
#include "nml_oi.hh"
#include "timer.hh"

/*
 * Include files required for all Toolkit programs
 */
#include <X11/Intrinsic.h>      /* Intrinsics Definitions */
#include <X11/StringDefs.h>     /* Standard Name-String definitions */

/*
 * Public include file for widgets we actually use in this file.
 */
//#include <X11/Xaw/Form.h>
//#include <X11/Xaw/Box.h>
//#include <X11/Xaw/Command.h>
//#include <X11/Xaw/Label.h>
//#include <X11/Xaw/AsciiText.h>
//#include <X11/Xaw/MenuButton.h>
//#include <X11/Xaw/SimpleMenu.h>
//#include <X11/Xaw/SmeBSB.h>
//#include <X11/Xaw/SmeLine.h>
//#include <X11/Xaw/Dialog.h>

#define UPDATE_MSECS 100
#define static_pzx
// the NML channels to the EMC task
static_pzx RCS_CMD_CHANNEL *emcCommandBuffer = 0;
static_pzx RCS_STAT_CHANNEL *emcStatusBuffer = 0;
EMC_STAT *emcStatus = 0;

// the NML channel for errors
static_pzx NML *emcErrorBuffer = 0;
static_pzx char error_string[NML_ERROR_LEN] = "";

// the current command numbers, set up updateStatus(), used in main()
static_pzx int emcCommandSerialNumber = 0;

// forward decls

// forward decls for error popup
static_pzx void errorReturnAction(Widget w, XEvent *event, String *params, Cardinal *num_params);
static_pzx void errorDoneCB(Widget w, XtPointer client_data, XtPointer call_data);
static_pzx int createErrorShell();
static_pzx int destroyErrorShell();
static_pzx void popupError(const char *fmt, ...) __attribute__((format(printf,1,2)));

// forward decl for quit() function
static_pzx void quit();

static_pzx int emcTaskNmlGet()
{
  int retval = 0;

  // try to connect to EMC cmd
  if (emcCommandBuffer == 0)
    {
      emcCommandBuffer = new RCS_CMD_CHANNEL(emcFormat, "emcCommand", "xemc", emc_nmlfile);
      if (! emcCommandBuffer->valid())
        {
          delete emcCommandBuffer;
          emcCommandBuffer = 0;
          retval = -1;
        }
    }

  // try to connect to EMC status
  if (emcStatusBuffer == 0)
    {
      emcStatusBuffer = new RCS_STAT_CHANNEL(emcFormat, "emcStatus", "xemc", emc_nmlfile);
      if (! emcStatusBuffer->valid() ||
          EMC_STAT_TYPE != emcStatusBuffer->peek())
        {
          delete emcStatusBuffer;
          emcStatusBuffer = 0;
          emcStatus = 0;
          retval = -1;
        }
      else
        {
          emcStatus = (EMC_STAT *) emcStatusBuffer->get_address();
        }
    }

  return retval;
}

static_pzx int emcErrorNmlGet()
{
  int retval = 0;

  if (emcErrorBuffer == 0)
    {
      emcErrorBuffer = new NML(nmlErrorFormat, "emcError", "xemc", emc_nmlfile);
      if (! emcErrorBuffer->valid())
        {
          delete emcErrorBuffer;
          emcErrorBuffer = 0;
          retval = -1;
        }
    }

  return retval;
}

static_pzx void printError(const char *error)
{
  printf("%s\n", error);
}

static_pzx int updateStatus()
{
  NMLTYPE type;

  if (0 == emcStatus ||
      0 == emcStatusBuffer ||
      ! emcStatusBuffer->valid())
    {
      return -1;
    }

  switch (type = emcStatusBuffer->peek())
    {
    case -1:
      // error on CMS channel
      return -1;
      break;

    case 0:                     // no new data
    case EMC_STAT_TYPE: // new data
      // new data
      break;

    default:
      return -1;
      break;
    }

  return 0;
}

static_pzx int updateError()
{
  NMLTYPE type;

  if (0 == emcErrorBuffer ||
      ! emcErrorBuffer->valid())
    {
      return -1;
    }

  switch (type = emcErrorBuffer->read())
    {
    case -1:
      // error reading channel
      return -1;
      break;

    case 0:
      // nothing new
      error_string[0] = 0;
      break;

    case EMC_OPERATOR_ERROR_TYPE:
      strncpy(error_string,
             ((EMC_OPERATOR_ERROR *) (emcErrorBuffer->get_address()))->error,
              LINELEN - 1);
      error_string[LINELEN - 1] = 0;
      break;

    case EMC_OPERATOR_TEXT_TYPE:
      strncpy(error_string,
             ((EMC_OPERATOR_TEXT *) (emcErrorBuffer->get_address()))->text,
              LINELEN - 1);
      error_string[LINELEN - 1] = 0;
      break;

    case EMC_OPERATOR_DISPLAY_TYPE:
      strncpy(error_string,
             ((EMC_OPERATOR_DISPLAY *) (emcErrorBuffer->get_address()))->display,
              LINELEN - 1);
      error_string[LINELEN - 1] = 0;
      break;

    case NML_ERROR_TYPE:
      strncpy(error_string,
             ((NML_ERROR *) (emcErrorBuffer->get_address()))->error,
              NML_ERROR_LEN - 1);
      error_string[NML_ERROR_LEN - 1] = 0;
      break;

    case NML_TEXT_TYPE:
      strncpy(error_string,
             ((NML_TEXT *) (emcErrorBuffer->get_address()))->text,
              NML_ERROR_LEN - 1);
      error_string[NML_ERROR_LEN - 1] = 0;
      break;

    case NML_DISPLAY_TYPE:
      strncpy(error_string,
             ((NML_DISPLAY *) (emcErrorBuffer->get_address()))->display,
              NML_ERROR_LEN - 1);
      error_string[NML_ERROR_LEN - 1] = 0;
      break;

    default:
      sprintf(error_string, "unrecognized error %" PRId32,type);
      return -1;
      break;
    }

  return 0;
}

#define EMC_COMMAND_TIMEOUT 5.0 // how long to wait until timeout
#define EMC_COMMAND_DELAY   0.1 // how long to sleep between checks

static_pzx int emcCommandWaitDone()
{
    double end;
    for (end = 0.0; end < EMC_COMMAND_TIMEOUT; end += EMC_COMMAND_DELAY) {
	updateStatus();
	int serial_diff = emcStatus->echo_serial_number - emcCommandSerialNumber;

	if (serial_diff < 0) {
	    continue;
	}

	if (serial_diff > 0) {
	    return 0;
	}

	if (emcStatus->status == RCS_DONE) {
	    return 0;
	}

	if (emcStatus->status == RCS_ERROR) {
	    return -1;
	}

	esleep(EMC_COMMAND_DELAY);
    }

    return -1;
}

static_pzx int emcCommandSend(RCS_CMD_MSG & cmd)
{
    // write command
    if (emcCommandBuffer->write(&cmd)) {
        return -1;
    }
    emcCommandSerialNumber = cmd.serial_number;

    // wait for receive
    double end;
    for (end = 0.0; end < EMC_COMMAND_TIMEOUT; end += EMC_COMMAND_DELAY) {
	updateStatus();
	int serial_diff = emcStatus->echo_serial_number - emcCommandSerialNumber;

	if (serial_diff >= 0) {
	    return 0;
	}

	esleep(EMC_COMMAND_DELAY);
    }

    return -1;
}


/*
  functions for handling the windowing of a file, where you
  give the line you wish to window and it produces an array
  of string, or single string, of the file's contents
  at that point and some lines beyond
  */

#define LF 10
#define CR 13

typedef struct {
  /* the array holding the window of lines in the file */
  char *fileWindow;

  /* the number of lines in the window */
  int maxWindowLines;

  /* the max length of each line */
  int maxLineLen;

  /* which array index holds the first filled slot */
  int windowStart;

  /* which array index holds the next open slot */
  int windowEnd;

  /* number in ring, also used to differentiate start = end as full/empty */
  int windowCount;

  /* the file to window */
  FILE * fileFp;

  /* the line currently at the top of the window */
  int fileFpLine;

  /* flag that the line should be kept */
  int keepNextLine;

} FILE_WINDOW;

static_pzx int fwClear(FILE_WINDOW *fw)
{
  int t;

  if (NULL == fw) {
    return -1;
  }

  for (t = 0; t < fw->maxWindowLines; t++) {
    fw->fileWindow[t * fw->maxLineLen] = 0;
  }

  fw->windowStart = 0;
  fw->windowEnd = 0;
  fw->windowCount = 0;

  fw->fileFpLine = 0;

  fw->keepNextLine = 1;

  return 0;
}

static_pzx int fwInit(FILE_WINDOW *fw, int _maxWindowLines, int _maxLineLen)
{
  if (NULL == fw) {
    return -1;
  }

  fw->fileWindow = (char *) malloc(_maxLineLen * _maxWindowLines);

  fw->maxWindowLines = _maxWindowLines;
  fw->maxLineLen = _maxLineLen;

  /* clear out the remaining vars */
  if (0 != fwClear(fw)) {
    return -1;
  }

  /* mark the file closed */
  fw->fileFp = NULL;

  return 0;
}

static_pzx int fwOpen(FILE_WINDOW *fw, const char *file)
{
  if (NULL == fw) {
    return -1;
  }

  /* close any open file */
  if (NULL != fw->fileFp) {
    fclose(fw->fileFp);
    fw->fileFp = NULL;
  }

  if (NULL == (fw->fileFp = fopen(file, "r"))) {
    return -1;
  }

  fw->keepNextLine = 1;

  return 0;
}

static_pzx int fwClose(FILE_WINDOW *fw)
{
  int retval = 0;

  if (NULL == fw) {
    return -1;
  }

  /* first clear out the window */
  if (0 != fwClear(fw)) {
    retval = -1;
  }

  if (NULL != fw->fileFp) {
    fclose(fw->fileFp);
  }
  fw->fileFp = NULL;

  return retval;
}

#if 0
static_pzx int fwDelete(FILE_WINDOW *fw)
{
  if (NULL == fw) {
    return -1;
  }

  /* should have been closed by call to fwClose(), but make sure */
  if (NULL != fw->fileFp) {
    fclose(fw->fileFp);
    fw->fileFp = NULL;
  }

  free(fw->fileWindow);

  return 0;
}

static_pzx int fwPrintWindow(FILE_WINDOW *fw)
{
  int start;

  if (NULL == fw) {
    return -1;
  }

  start = fw->windowStart;

  if (0 == fw->windowCount) {
    return 0;
  }

  do {
    printf("%s\n", &(fw->fileWindow[start * fw->maxLineLen]));
    if (++start >= fw->maxWindowLines) {
      start = 0;
    }
  } while (start != fw->windowEnd);

  return 0;
}
#endif

static_pzx int fwAddLine(FILE_WINDOW *fw, const char *line)
{
  if (NULL == fw) {
    return -1;
  }

  strncpy(&fw->fileWindow[fw->windowEnd * fw->maxLineLen], line, fw->maxLineLen - 1);
  fw->fileWindow[fw->windowEnd * fw->maxLineLen + fw->maxLineLen - 1] = 0;

  if (fw->windowEnd == fw->windowStart &&
      0 != fw->windowCount) {
    /* we're full, so move fw->windowStart up */
    /* and don't increment fw->windowCount */
    if (++fw->windowStart >= fw->maxWindowLines) {
      fw->windowStart = 0;
    }
  }
  else {
    /* we're not full, so no need to move fw->windowStart up */
    /* but do increment fw->windowCount */
    ++fw->windowCount;
  }

  /* now increment fw->windowEnd to point to next slot */
  if (++fw->windowEnd >= fw->maxWindowLines) {
    fw->windowEnd = 0;
  }

  return 0;
}

#if 0
static_pzx int fwDeleteLine(FILE_WINDOW *fw)
{
  if (NULL == fw) {
    return -1;
  }

  if (0 == fw->windowCount) {
    return 0;
  }

  fw->fileWindow[fw->windowStart * fw->maxLineLen] = 0;
  if (++fw->windowStart >= fw->maxWindowLines) {
    fw->windowStart = 0;
  }

  --fw->windowCount;

  return 0;
}
#endif

static_pzx int fwSyncLine(FILE_WINDOW *fw, int syncLine)
{
  char line[256];               // FIXME-- hardcoded
  int pad;
  int len;
  int sawEol;

  if (NULL == fw) {
    return -1;
  }

  if (NULL == fw->fileFp) {
    return -1;
  }

  /* if syncLine is <= 0, make it 1 */
  if (syncLine <= 0) {
    syncLine = 1;
  }

  /* reset syncLine so that it means the first line is synched */
  syncLine += fw->maxWindowLines - 1;

  /* check if our window is ahead of file, and rewind if so */
  if (syncLine < fw->fileFpLine) {
    /* we're ahead of program, so rewind */
    rewind(fw->fileFp);
    /* and clear out fw->fileWindow */
    fwClear(fw);
  }

  /* now the window is at or behind the file */
  /* so fill it up */
  while (fw->fileFpLine < syncLine) {
    if (NULL == fgets(line, fw->maxLineLen, fw->fileFp)) {
      /* end file */
      /* pad remainder if any */
      pad = syncLine - fw->fileFpLine;
      while (pad-- > 0) {
        fwAddLine(fw, "");
      }
      fw->fileFpLine = syncLine;
      break;
    }

    sawEol = 0;
    len = strlen(line);
    while (--len >= 0) {
      if (CR == line[len] ||
          LF == line[len]) {
        line[len] = 0;
        sawEol = 1;
      }
      else {
        break;
      }
    }

    if (fw->keepNextLine) {
      fwAddLine(fw, line);
      ++fw->fileFpLine;
    }

    fw->keepNextLine = sawEol;
  }

  return 0;
}

static_pzx int fwString(FILE_WINDOW *fw, char *string)
{
  int start;

  if (NULL == fw) {
    return -1;
  }

  start = fw->windowStart;
  string[0] = 0;

  if (0 == fw->windowCount) {
    return 0;
  }

  do {
    strncat(string, &(fw->fileWindow[start * fw->maxLineLen]),
            fw->maxLineLen - 2);
    strcat(string, "\n");
    if (++start >= fw->maxWindowLines) {
      start = 0;
    }
  } while (start != fw->windowEnd);

  return 0;
}

// the file window structure for the program window and related stuff

#define PROGRAM_FW_NUM_LINES 10
#define PROGRAM_FW_LEN_LINES 80

static_pzx char *programFwString = NULL;
static_pzx FILE_WINDOW programFw;

// number of axes supported
#define XEMC_NUM_AXES 9

// string for ini file version num
static_pzx char version_string[LINELEN] = "";

// interpreter parameter file name, from ini file
static_pzx char PARAMETER_FILE[LINELEN] = "";

// help file name, from ini file
static_pzx char HELP_FILE[LINELEN] = "";

// the program path prefix
static_pzx char programPrefix[LINELEN] = "";
// the program name currently displayed
static_pzx char programFile[LINELEN] = "*";
// the program last loaded by the controller
static_pzx char lastProgramFile[LINELEN] = "*";

// integer version of ini file max scale factor
static_pzx int maxFeedOverride = 100;

#define MDI_LINELEN 80
static_pzx char active_g_codes_string[MDI_LINELEN] = "";
static_pzx char active_m_codes_string[MDI_LINELEN] = "";

// how position is to be displayed-- relative or machine
typedef enum {
  COORD_RELATIVE = 1,
  COORD_MACHINE
} COORD_TYPE;

static_pzx COORD_TYPE coords = COORD_RELATIVE;

// how position is to be displayed-- actual or commanded
typedef enum {
  POS_DISPLAY_ACT = 1,
  POS_DISPLAY_CMD
} POS_DISPLAY_TYPE;

static_pzx POS_DISPLAY_TYPE posDisplay = POS_DISPLAY_ACT;

// marker for the active axis
static_pzx int activeAxis = 0;      // default is 0, X
static_pzx int oldActiveAxis = -1;  // force an update at startup

/*
  Note: the X key press/release events with multiple keys behave such that
  if key a is pressed, then b, then a released, its key-release event
  won't go through. So, multi-axis jogging has been disallowed in xemc,
  although it's supported in the motion system.
*/
// flag that an axis is jogging, so other jogs won't go out
static_pzx int axisJogging = -1;

// current jog speed setting
static_pzx int jogSpeed = 1;        // overridden in iniLoad()
static_pzx int maxJogSpeed = 1;     // overridden in iniLoad()
static_pzx int jogSpeedChange = 0;  // 0 means no change, +/- means inc/dec

// current jog increment setting, non-positive means continuous
static_pzx double jogIncrement = 0.0;

// the size of the smallest increment to jog, = 1/INPUT_SCALE
static_pzx double stepIncrement = 0.0001;

// polarities for axis jogging, from ini file
static_pzx int jogPol[XEMC_NUM_AXES];

static_pzx int oldFeedOverride = -1; // forces an update at startup
static_pzx int feedOverride;        // 100% integer copy of EMC status
static_pzx int feedOverrideChange = 0; // same as jogSpeedChange
#define FEED_OVERRIDE_DELAY_COUNT 1
// timer delays until dec/inc appears
static_pzx int feedOverrideDelayCount = FEED_OVERRIDE_DELAY_COUNT;

// command sending functions

static_pzx int sendEstop()
{
  EMC_TASK_SET_STATE state_msg;

  state_msg.state = EMC_TASK_STATE_ESTOP;
  emcCommandSend(state_msg);

  return 0;
}

static_pzx int sendEstopReset()
{
  EMC_TASK_SET_STATE state_msg;

  state_msg.state = EMC_TASK_STATE_ESTOP_RESET;
  emcCommandSend(state_msg);

  return 0;
}

static_pzx int sendMachineOn()
{
  EMC_TASK_SET_STATE state_msg;

  state_msg.state = EMC_TASK_STATE_ON;
  emcCommandSend(state_msg);

  return 0;
}

static_pzx int sendMachineOff()
{
  EMC_TASK_SET_STATE state_msg;

  state_msg.state = EMC_TASK_STATE_OFF;
  emcCommandSend(state_msg);

  return 0;
}

static_pzx int sendManual()
{
  EMC_TASK_SET_MODE mode_msg;

  mode_msg.mode = EMC_TASK_MODE_MANUAL;
  emcCommandSend(mode_msg);

  return 0;
}

static_pzx int sendAuto()
{
  EMC_TASK_SET_MODE mode_msg;

  mode_msg.mode = EMC_TASK_MODE_AUTO;
  emcCommandSend(mode_msg);

  return 0;
}

static_pzx int sendMdi()
{
  EMC_TASK_SET_MODE mode_msg;

  mode_msg.mode = EMC_TASK_MODE_MDI;
  emcCommandSend(mode_msg);

  return 0;
}

static_pzx int sendToolSetOffset(int toolno, double zoffset, double diameter)
{
  EMC_TOOL_SET_OFFSET emc_tool_set_offset_msg;

  emc_tool_set_offset_msg.toolno = toolno;
  emc_tool_set_offset_msg.offset.tran.z = zoffset;
  emc_tool_set_offset_msg.diameter = diameter;
  emc_tool_set_offset_msg.orientation = 0; // mill style tool table

  emcCommandSend(emc_tool_set_offset_msg);

  return 0;
}

static_pzx int sendMistOn()
{
  EMC_COOLANT_MIST_ON emc_coolant_mist_on_msg;

  emcCommandSend(emc_coolant_mist_on_msg);

  return 0;
}

static_pzx int sendMistOff()
{
  EMC_COOLANT_MIST_OFF emc_coolant_mist_off_msg;

  emcCommandSend(emc_coolant_mist_off_msg);

  return 0;
}

static_pzx int sendFloodOn()
{
  EMC_COOLANT_FLOOD_ON emc_coolant_flood_on_msg;

  emcCommandSend(emc_coolant_flood_on_msg);

  return 0;
}

static_pzx int sendFloodOff()
{
  EMC_COOLANT_FLOOD_OFF emc_coolant_flood_off_msg;

  emcCommandSend(emc_coolant_flood_off_msg);

  return 0;
}

static_pzx int sendSpindleForward()
{
  EMC_SPINDLE_ON emc_spindle_on_msg;

  emc_spindle_on_msg.speed = +1;
  emcCommandSend(emc_spindle_on_msg);

  return 0;
}

static_pzx int sendSpindleReverse()
{
  EMC_SPINDLE_ON emc_spindle_on_msg;

  emc_spindle_on_msg.speed = -1;
  emcCommandSend(emc_spindle_on_msg);

  return 0;
}

static_pzx int sendSpindleOff()
{
  EMC_SPINDLE_OFF emc_spindle_off_msg;

  emcCommandSend(emc_spindle_off_msg);

  return 0;
}

static_pzx int sendSpindleIncrease()
{
  EMC_SPINDLE_INCREASE emc_spindle_increase_msg;

  emcCommandSend(emc_spindle_increase_msg);

  return 0;
}

static_pzx int sendSpindleDecrease()
{
  EMC_SPINDLE_DECREASE emc_spindle_decrease_msg;

  emcCommandSend(emc_spindle_decrease_msg);

  return 0;
}

static_pzx int sendSpindleConstant()
{
  EMC_SPINDLE_CONSTANT emc_spindle_constant_msg;

  emcCommandSend(emc_spindle_constant_msg);

  return 0;
}

static_pzx int sendBrakeEngage()
{
  EMC_SPINDLE_BRAKE_ENGAGE emc_spindle_brake_engage_msg;

  emcCommandSend(emc_spindle_brake_engage_msg);

  return 0;
}

static_pzx int sendBrakeRelease()
{
  EMC_SPINDLE_BRAKE_RELEASE emc_spindle_brake_release_msg;

  emcCommandSend(emc_spindle_brake_release_msg);

  return 0;
}

static_pzx int sendAbort()
{
  EMC_TASK_ABORT task_abort_msg;

  emcCommandSend(task_abort_msg);

  return 0;
}

static_pzx int sendOverrideLimits()
{
  EMC_AXIS_OVERRIDE_LIMITS lim_msg;

  lim_msg.axis = 0;             // same number for all
  emcCommandSend(lim_msg);

  return 0;
}

static_pzx int sendJogStop(int axis)
{
  EMC_AXIS_ABORT emc_axis_abort_msg;

  if (axis < 0 || axis >= XEMC_NUM_AXES) {
    return -1;
  }

  // don't send request to jog if none are jogging
  if (axisJogging == -1) {
    return 0;
  }

  emc_axis_abort_msg.axis = axisJogging;
  emcCommandSend(emc_axis_abort_msg);

  axisJogging = -1;

  return 0;
}

static_pzx int sendJogCont(int axis, double speed)
{
  EMC_AXIS_JOG emc_axis_jog_msg;

  if (axis < 0 || axis >= XEMC_NUM_AXES) {
    return -1;
  }

  if (axisJogging != -1) {
    // ignore request to jog, if an axis is already jogging
    return 0;
  }

  if (0 == jogPol[axis]) {
    speed = -speed;
  }

  emc_axis_jog_msg.axis = axis;
  emc_axis_jog_msg.vel = speed / 60.0;
  emcCommandSend(emc_axis_jog_msg);

  axisJogging = axis;

  return 0;
}

static_pzx int sendJogIncr(int axis, double speed, double incr)
{
  EMC_AXIS_INCR_JOG emc_axis_incr_jog_msg;

  if (axis < 0 || axis >= XEMC_NUM_AXES) {
    return -1;
  }

  if (axisJogging != -1) {
    // ignore request to jog, if an axis is already jogging
    return 0;
  }

  if (0 == jogPol[axis]) {
    speed = -speed;
  }

  emc_axis_incr_jog_msg.axis = axis;
  emc_axis_incr_jog_msg.vel = speed / 60.0;
  emc_axis_incr_jog_msg.incr = jogIncrement;
  emcCommandSend(emc_axis_incr_jog_msg);

  // don't flag incremental jogs as jogging an axis-- we can
  // allow multiple incremental jogs since we don't need a key release

  return 0;
}

static_pzx int sendHome(int axis)
{
  EMC_AXIS_HOME emc_axis_home_msg;

  emc_axis_home_msg.axis = axis;
  emcCommandSend(emc_axis_home_msg);

  return 0;
}

static_pzx int sendFeedOverride(double override)
{
  EMC_TRAJ_SET_SCALE emc_traj_set_scale_msg;

  if (override < 0.0) {
    override = 0.0;
  }
  else if (override > (double) maxFeedOverride / 100.0) {
    override = (double) maxFeedOverride / 100.0;
  }
  emc_traj_set_scale_msg.scale = override;
  emcCommandSend(emc_traj_set_scale_msg);

  return 0;
}

static_pzx int sendTaskPlanInit()
{
  EMC_TASK_PLAN_INIT task_plan_init_msg;

  emcCommandSend(task_plan_init_msg);

  return 0;
}

static_pzx int sendProgramOpen(char *program)
{
  EMC_TASK_PLAN_OPEN emc_task_plan_open_msg;

  // first put in auto mode if it's not
  if (EMC_TASK_MODE_AUTO != emcStatus->task.mode) {
    // send a request to go to auto mode
    sendAuto();
  }

  // wait for any previous one to go out
  if (0 != emcCommandWaitDone()) {
    printError("error executing command\n");
    return -1;
  }

  strcpy(emc_task_plan_open_msg.file, program);
  emcCommandSend(emc_task_plan_open_msg);

  // now clear out our stored version of the program, in case
  // the file contents have changed but the name is the same
  programFile[0] = 0;

  return 0;
}

// line in program to run from; set it in GUI when user clicks on a line,
// and pass it in calls to sendProgramRun(). sendProgramRun() won't use
// this directly.
static_pzx int programStartLine = 0;
static_pzx int programStartLineLast = 0;

static_pzx int sendProgramRun(int line)
{
  EMC_TASK_PLAN_RUN emc_task_plan_run_msg;

  // first reopen program if it's not open
  if (0 == emcStatus->task.file[0]) {
    // send a request to open last one
    sendProgramOpen(lastProgramFile);

    // wait for command to go out
    if (0 != emcCommandWaitDone()) {
      printError("error executing command\n");
      return -1;
    }
  }

  emc_task_plan_run_msg.line = line;
  emcCommandSend(emc_task_plan_run_msg);

  programStartLineLast = programStartLine;
  programStartLine = 0;

  return 0;
}

static_pzx int sendProgramPause()
{
  EMC_TASK_PLAN_PAUSE emc_task_plan_pause_msg;

  emcCommandSend(emc_task_plan_pause_msg);

  return 0;
}

static_pzx int sendProgramResume()
{
  EMC_TASK_PLAN_RESUME emc_task_plan_resume_msg;

  emcCommandSend(emc_task_plan_resume_msg);

  return 0;
}

static_pzx int sendProgramStep()
{
  EMC_TASK_PLAN_STEP emc_task_plan_step_msg;

  // first reopen program if it's not open
  if (0 == emcStatus->task.file[0]) {
    // send a request to open last one
    sendProgramOpen(lastProgramFile);

    // wait for command to go out
    if (0 != emcCommandWaitDone()) {
      printError("error executing command\n");
      return -1;
    }
  }

  emcCommandSend(emc_task_plan_step_msg);

  programStartLineLast = programStartLine;
  programStartLine = 0;

  return 0;
}

static_pzx int sendMdiCmd(char *mdi)
{
  EMC_TASK_PLAN_EXECUTE emc_task_plan_execute_msg;

  strcpy(emc_task_plan_execute_msg.command, mdi);
  emcCommandSend(emc_task_plan_execute_msg);

  return 0;
}

static_pzx int sendLoadToolTable(const char *file)
{
  EMC_TOOL_LOAD_TOOL_TABLE emc_tool_load_tool_table_msg;

  strcpy(emc_tool_load_tool_table_msg.file, file);
  emcCommandSend(emc_tool_load_tool_table_msg);

  return 0;
}

static_pzx char stepIncrementLabel[32] = "0.123456";

// destructively converts string to its uppercase counterpart
static_pzx char *upcase(char *string)
{
    char *ptr = string;

    while (*ptr != 0)
    {
        *ptr = toupper(*ptr);
        ptr++;
    }

    return string;
}

//read nml to global
static_pzx int iniLoad(const char *filename)
{
    IniFile inifile;
    const char *inistring;
    char machine[LINELEN] = "";
    char version[LINELEN] = "";
    char displayString[LINELEN] = "";
    int t;
    int i;
    double d;

// open nml
    if (!inifile.Open(filename))
    {
        return -1;
    }
//get MACHINE (in EMC)
    if (NULL != (inistring = inifile.Find("MACHINE", "EMC")))
    {
        strcpy(machine, inistring);

        if (NULL != (inistring = inifile.Find("VERSION", "EMC")))
        {
          sscanf(inistring, "$Revision: %s", version);

          sprintf(version_string, "%s EMC Version %s", machine, version);
        }
    }
//open debug model? (in EMC)
    if (NULL != (inistring = inifile.Find("DEBUG", "EMC")))
    {
        // copy to global
        if (1 != sscanf(inistring, "%i", &emc_debug))
        {
          emc_debug = 0;
        }
    }
    else
    {
        // not found, use default
        emc_debug = 0;
    }
//get NML_FILE (in EMC)
    if (NULL != (inistring = inifile.Find("NML_FILE", "EMC")))
    {
        // copy to global
        strcpy(emc_nmlfile, inistring);
    }
    else {
        // not found, use default
    }
//get TOOL_TABLE (in EMCIO)
    if (NULL != (inistring = inifile.Find("TOOL_TABLE", "EMCIO")))
    {
        strcpy(tool_table_file, inistring);
    }
    else
    {
        strcpy(tool_table_file, "tool.tbl"); // FIXME-- hardcoded
    }
//get PARAMETER_FILE (in RS274NGC)
    if (NULL != (inistring = inifile.Find("PARAMETER_FILE", "RS274NGC"))) {
    strcpy(PARAMETER_FILE, inistring);
    }
    else {
    strcpy(PARAMETER_FILE, "rs274ngc.var"); // FIXME-- hardcoded
    }
//get DEFAULT_VELOCITY (in TRAJ)
    if (NULL != (inistring = inifile.Find("DEFAULT_VELOCITY", "TRAJ")))
    {
        if (1 != sscanf(inistring, "%lf", &traj_default_velocity))
        {
          traj_default_velocity = DEFAULT_TRAJ_DEFAULT_VELOCITY;
        }
    }
    else
    {
        traj_default_velocity = DEFAULT_TRAJ_DEFAULT_VELOCITY;
    }
// round *jogSpeed in display to integer, per-minute
    jogSpeed = (int) (traj_default_velocity * 60.0 + 0.5);
//get DEFAULT_VELOCITY (in TRAJ)
    if (NULL != (inistring = inifile.Find("MAX_VELOCITY", "TRAJ")))
    {
        if (1 != sscanf(inistring, "%lf", &traj_max_velocity))
        {
          traj_max_velocity = DEFAULT_TRAJ_MAX_VELOCITY;
        }
    }
    else
    {
         traj_max_velocity = DEFAULT_TRAJ_MAX_VELOCITY;
    }
// round *maxJogSpeed in display to integer, per-minute
    maxJogSpeed = (int) (traj_max_velocity * 60.0 + 0.5);
//get HELP_FILE (in DISPLAY)
    if (NULL != (inistring = inifile.Find("HELP_FILE", "DISPLAY")))
    {
        strcpy(HELP_FILE, inistring);
    }
//get PROGRAM_PREFIX (in DISPLAY)
    if (NULL != (inistring = inifile.Find("PROGRAM_PREFIX", "DISPLAY")))
    {
        if (1 != sscanf(inistring, "%s", programPrefix))
        {
          programPrefix[0] = 0;
        }
    }
    else if (NULL != (inistring = inifile.Find("PROGRAM_PREFIX", "TASK")))
    {
        if (1 != sscanf(inistring, "%s", programPrefix))
        {
            programPrefix[0] = 0;
        }
    }
    else
    {
          programPrefix[0] = 0;
    }
//get POSITION_OFFSET (in DISPLAY)
    if (NULL != (inistring = inifile.Find("POSITION_OFFSET", "DISPLAY")))
    {
        if (1 == sscanf(inistring, "%s", displayString))
        {
            if (! strcmp(upcase(displayString), "MACHINE"))
            {
                coords = COORD_MACHINE;
            }
            else if (1 == sscanf(inistring, "%s", displayString))
            {
                if (! strcmp(upcase(displayString), "RELATIVE"))
                {
                  coords = COORD_RELATIVE;
                }
            }
            else
            {
                // error-- invalid value
                // ignore
             }
        }
        else {
          // error-- no value provided
          // ignore
        }
    }
    else {
    // no line at all
    // ignore
    }
//get POSITION_FEEDBACK (in DISPLAY)
    if (NULL != (inistring = inifile.Find("POSITION_FEEDBACK", "DISPLAY")))
    {
        if (1 == sscanf(inistring, "%s", displayString))
        {
            if (! strcmp(upcase(displayString), "ACTUAL"))
            {
                posDisplay = POS_DISPLAY_ACT;
            }
            else if (1 == sscanf(inistring, "%s", displayString))
            {
                if (! strcmp(upcase(displayString), "COMMANDED"))
                {
                    posDisplay = POS_DISPLAY_CMD;
                }
            }
            else
            {
                // error-- invalid value
                // ignore
            }
        }
        else
        {
          // error-- no value provided
          // ignore
        }
    }
    else
    {
    // no line at all
    // ignore
    }
//get JOGGING_POLARITY (in displayString)
    for (t = 0; t < XEMC_NUM_AXES; t++)
    {
        jogPol[t] = 1;              // set to default
        sprintf(displayString, "AXIS_%d", t);
        if (NULL != (inistring = inifile.Find("JOGGING_POLARITY", displayString)) &&
            1 == sscanf(inistring, "%d", &i) &&
            i == 0)
        {
            // it read as 0, so override default
            jogPol[t] = 0;
        }
    }
//get MAX_FEED_OVERRIDE (in DISPLAY)
    if (NULL != (inistring = inifile.Find("MAX_FEED_OVERRIDE", "DISPLAY")))
    {
        if (1 == sscanf(inistring, "%lf", &d) && d > 0.0)
        {
          maxFeedOverride = (int) (d * 100.0 + 0.5);
        }
        else
        {
          // error-- no value provided
          // ignore
        }
    }
    else {
    // no line at all
    // ignore
    }


    // FIXME-- we're using the first axis scale to set the jog increment.
    // Note that stepIncrement is inited to a reasonable value above, and
    // will only be reset on a good ini file match

//get INPUT_SCALE (in AXIS_0)
    if (NULL != (inistring = inifile.Find("INPUT_SCALE", "AXIS_0")))
    {
        if (1 == sscanf(inistring, "%lf", &d))
        {
            if (d < 0.0)
            {
                stepIncrement = -1.0/d; // posify it
            }
            else if (d > 0.0)
            {
                stepIncrement = 1.0/d;
            }
            // else it's 0, so ignore (this will kill the EMC, by the way)
        }
    }

    // set step increment to be less than 0.0010, the last fixed increment,
    // if it's larger. Set to 0.0001 if so, which will be too small but it
    // can't hurt.

    if (stepIncrement >= 0.0010)
    {
        stepIncrement = 0.0001;
    }
    sprintf(stepIncrementLabel, "%.6f", stepIncrement);

    // close it
    inifile.Close();

    return 0;
}

int main(int argc, char **argv)
{
    int               t         ;
    double            start     ;
    int               good      ;
    char              string[80];
    Dimension         cmfbw, sdw, sw, siw, sh, bh, bw;
    Dimension         stw, mw;
    Dimension         posw;
    Font              posfont;

    // process command line args
    if (0 != emcGetArgs(argc, argv))
    {
        rcs_print_error("error in argument list\n");
        //    exit(1);
    }

    // read INI file
    iniLoad(emc_inifile);
    // init NML

    #define RETRY_TIME 10.0         // seconds to wait for subsystems to come up
    #define RETRY_INTERVAL 1.0      // seconds between wait tries for a subsystem

//    emc_debug = 0x00000040;
    if (! (emc_debug & EMC_DEBUG_NML))
    {
        set_rcs_print_destination(RCS_PRINT_TO_NULL);     // inhibit diag messages
    }

    start = etime();
    rcs_print_error("[%s]-[%d]-[%s]-[start:%f]\n ",__FILE__, __LINE__, __FUNCTION__,start);

    good = 0;

    do {
        if (0 == emcTaskNmlGet())
        {
            good = 1;
            break;
        }
        esleep(RETRY_INTERVAL);
    } while ( etime() - start  <  RETRY_TIME );

    if (! (emc_debug & EMC_DEBUG_NML))
    {
        set_rcs_print_destination(RCS_PRINT_TO_STDOUT); // restore diag messages
    }

    if (! good)
    {
        rcs_print_error("can't establish communication with emc\n");
//    exit(1);
    }

    if (! (emc_debug & EMC_DEBUG_NML))
    {
      set_rcs_print_destination(RCS_PRINT_TO_NULL);     // inhibit diag messages
    }
    start = etime();
    good = 0;

    do {
        if (0 == emcErrorNmlGet())
        {
            good = 1;
            break;
        }
        esleep(RETRY_INTERVAL);
    } while (etime() - start < RETRY_TIME);

    if (! (emc_debug & EMC_DEBUG_NML))
    {
      set_rcs_print_destination(RCS_PRINT_TO_STDOUT); // restore diag messages
    }

    if (! good)
    {
        rcs_print_error("can't establish communication with emc\n");
    //    exit(1);
    }

  // create file window for program text

    programFwString =  (char *) malloc(PROGRAM_FW_NUM_LINES * PROGRAM_FW_LEN_LINES);

    if (0 != fwInit(&programFw, PROGRAM_FW_NUM_LINES, PROGRAM_FW_LEN_LINES))
    {
        fprintf(stderr, "can't init file window\n");
//    exit(1);
    }

    //first test
    {
        rcs_print_error("mycnc_pzx let we go\n");

        //sendEstopReset();
        //sendMachineOn();
        //sendManual();

        rcs_print_error("mycnc_pzx let we sendManual\n");

        //sendJogCont(1,100);
        rcs_print_error("mycnc_pzx let we axis 1\n");
        //sendJogCont(0,100);
        rcs_print_error("mycnc_pzx let we axis 2\n");
        //sendJogCont(2,100);
        rcs_print_error("mycnc_pzx let we axis 3\n");
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();

    return 0;
}
