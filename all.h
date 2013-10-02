
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#ifdef RPI_NO_X
#include <linux/input.h>
#endif

#include <png.h>
#include <tcl.h>
#include <curl/curl.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>


#ifdef OSX
#include <SDL_main.h>
#endif
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_events.h>
#ifndef SDLGL
#include "esUtil.h"
#else
#ifndef OSX
#define NO_SDL_GLEXT
#include <GL/glew.h>
#include <GL/glext.h>
#include <GL/gl.h>
#else
#include <OpenGL/glu.h>
#endif
#include <SDL_image.h>
#include <SDL_opengl.h>
#endif

#ifndef OSX
#ifdef SDLGL
#include <videocapture.h>
#include <savepng.h>
#endif
#endif

#include <model.h>
#include <serial.h>
#include <geomag70.h>
#include <webclient.h>
#include <my_mavlink.h>
#include <userdata.h>
#include <main.h>
#include <my_gps.h>
#include <mwi21.h>
#include <simplebgc.h>
#include <brugi.h>
#include <openpilot.h>
#include <jeti.h>
#include <frsky.h>
#include <tracker.h>
#include <webserv.h>
#include <logging.h>
#include <openpilot_xml.h>

#include <screen_background.h>
#include <screen_baseflightcli.h>
#include <screen_baud.h>
#include <screen_brugi.h>
#include <screen_calibration.h>
#include <screen_cli.h>
#include <screen_device.h>
#include <screen_filesystem.h>
#include <screen_fms.h>
#include <screen_graph.h>
#include <screen_hud.h>
#include <screen_keyboard.h>
#include <screen_map.h>
#include <screen_mavlink_menu.h>
#include <screen_model.h>
#include <screen_mwi_menu.h>
#include <screen_openpilot_menu.h>
#include <screen_rcflow.h>
#include <screen_system.h>
#include <screen_tcl.h>
#include <screen_telemetry.h>
#include <screen_tracker.h>
#include <screen_videolist.h>
#include <screen_wpedit.h>



