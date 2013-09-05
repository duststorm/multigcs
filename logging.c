
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#include <math.h>
#include <linux/version.h>
#include <linux/input.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>
#include <termios.h>

#include <png.h>
#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_events.h>

#ifndef SDLGL
#include "esUtil.h"
#else
#include <SDL_image.h>
#include <SDL_opengl.h>
#include "GL/gl.h"
#include "GL/glext.h"
#endif

#include <model.h>

#include <geomag70.h>

#include "my_mavlink.h"
#include <userdata.h>
#include <main.h>
#include "my_gps.h"
#include "mwi21.h"
#include "simplebgc.h"
#include "brugi.h"
#include "openpilot.h"
#include "jeti.h"
#include "frsky.h"
#include "webserv.h"
#include "logging.h"

char *Logdata = NULL;
int log_startup_time = 0;
uint8_t logmode = LOGGING_OFF;
uint8_t logplay_open = 0;
uint8_t logplay_play = 0;
uint8_t logplay_pause = 0;
uint32_t logplay_msec = 0;
uint32_t logplay_startsec = 0;
uint8_t logplay_list = 0;
uint8_t logplay_filelist = 0;
char logplay_file[1024];


uint8_t logging_set_mode (char *name, float x, float y, int8_t button, float data) {
	if (logmode == 0) {
		logmode = 1;
		LogSave();
		LogInit();
	} else if (logmode == 1) {
		LogSave();
		logmode = 2;
	} else {
		LogSave();
		logmode = 0;
	}
	return 0;
}


void logplay_export_kml (char *logfile, char *kmlfile) {
	printf("logplay: %s -> %s\n", logfile, kmlfile);
	if (logfile[0] != 0) {
		float p_lat = 0.0;
		float p_long = 0.0;
		float p_alt = 0.0;
		float pitch = 0.0;
		float roll = 0.0;
		float yaw = 0.0;
		float speed = 0.0;
		uint8_t gpsfix = 0;
		uint8_t numSat = 0;
		char line[512];
		char filename[512];
		uint32_t lsec = 0;
		uint32_t lmicros = 0;
		float last_p_long = 0.0;
		float last_p_lat = 0.0;
		float last_p_alt = 0.0;
		uint16_t point_nr = 0;
	        FILE *fr;
		FILE *log_fr = NULL;
		log_fr = fopen(logfile, "r");
		if (log_fr != 0) {
			sprintf(filename, kmlfile);
		        fr = fopen(filename, "w");
			if (fr != 0) {
				fprintf(fr, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
//				fprintf(fr, "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n");
				fprintf(fr, "<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\n");

				fprintf(fr, "  <Document>\n");
//				fprintf(fr, "    <name>%s</name>\n", rctransmitter_name);
				fprintf(fr, "    <description>Flight-Data</description>\n");


				fprintf(fr, "   <Folder>\n");
				fprintf(fr, "    <name>Route</name>\n");
				fprintf(fr, "    <Style id=\"yellowLineGreenPoly\">\n");
				fprintf(fr, "      <LineStyle>\n");
				fprintf(fr, "        <color>7f00ffff</color>\n");
				fprintf(fr, "        <width>4</width>\n");
				fprintf(fr, "      </LineStyle>\n");
				fprintf(fr, "      <PolyStyle>\n");
				fprintf(fr, "        <color>7f00ff00</color>\n");
				fprintf(fr, "      </PolyStyle>\n");
				fprintf(fr, "    </Style>\n");
				fprintf(fr, "    <Placemark>\n");
				fprintf(fr, "      <name>Absolute Extruded</name>\n");
				fprintf(fr, "      <description>Datalog</description>\n");
				fprintf(fr, "      <styleUrl>#yellowLineGreenPoly</styleUrl>\n");
				fprintf(fr, "      <LineString>\n");
				fprintf(fr, "        <extrude>1</extrude>\n");
				fprintf(fr, "        <tessellate>1</tessellate>\n");
				fprintf(fr, "        <altitudeMode>absolute</altitudeMode>\n");
//				fprintf(fr, "        <altitudeMode>relativeToGround</altitudeMode>\n");
				fprintf(fr, "        <coordinates>");
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
						fprintf(fr, " %f,%f,%f \n", p_long, p_lat, p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);
					}
				}
				fprintf(fr, "        </coordinates>\n");
				fprintf(fr, "      </LineString>\n");
				fprintf(fr, "    </Placemark>\n");
				fprintf(fr, "   </Folder>\n");

/*
				fprintf(fr, "   <Folder>\n");
				fprintf(fr, "    <name>Arrows</name>\n");
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						last_p_long = p_long;
						last_p_lat = p_lat;
						last_p_alt = p_alt;
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);

						fprintf(fr, "    <Style id=\"STYLE%i\">\n", point_nr);
						fprintf(fr, "      <LabelStyle>\n");
						fprintf(fr, "        <scale>0.0</scale>\n");
						fprintf(fr, "      </LabelStyle>\n");
						fprintf(fr, "      <IconStyle>\n");
						fprintf(fr, "        <scale>0.8</scale>\n");
						fprintf(fr, "        <Icon>\n");
						fprintf(fr, "          <href>http://www.earthpoint.us/Dots/GoogleEarth/shapes/arrow.png</href>\n");
						fprintf(fr, "        </Icon>\n");
//						fprintf(fr, "        <tilt>%f</tilt>\n", pitch + 90.0);
//						fprintf(fr, "        <roll>%f</roll>\n", roll);
						fprintf(fr, "        <heading>%f</heading>\n", yaw - 180.0);
						fprintf(fr, "        <color>FFFF0000</color>\n");
						fprintf(fr, "      </IconStyle>\n");
						fprintf(fr, "    </Style>\n");

						fprintf(fr, "    <Placemark>\n");
						fprintf(fr, "      <name>%i</name>\n", point_nr);
						fprintf(fr, "      <styleUrl>#STYLE%i</styleUrl>\n", point_nr);
						fprintf(fr, "      <description><![CDATA[%i.%03i<BR>alt=%fm<BR>pitch=%f<BR>roll=%f<BR>yaw=%f]]></description>\n", lsec, lmicros, p_alt, pitch, roll, yaw);
						fprintf(fr, "      <Point>\n");
						fprintf(fr, "        <coordinates>%f, %f, %f</coordinates>\n", p_long, p_lat, p_alt);
						fprintf(fr, "      </Point>\n");
						fprintf(fr, "      <LookAt>\n");
						fprintf(fr, "        <longitude>%f</longitude>\n", last_p_long);
						fprintf(fr, "        <latitude>%f</latitude>\n", last_p_lat);
						fprintf(fr, "        <altitude>%f</altitude>\n", last_p_alt);
						fprintf(fr, "        <heading>%f</heading>\n", yaw);
						fprintf(fr, "        <tilt>%f</tilt>\n", pitch + 90.0);
						fprintf(fr, "        <roll>%f</roll>\n", roll);
						fprintf(fr, "        <altitudeMode>absolute</altitudeMode>\n");
						fprintf(fr, "      </LookAt>\n");
						fprintf(fr, "    </Placemark>\n");

						point_nr++;
					}
				}
				fprintf(fr, "   </Folder>\n");
*/

/*

				fprintf(fr, "    <Style id=\"multiTrack_n\">\n");
				fprintf(fr, "      <IconStyle>\n");
				fprintf(fr, "        <Icon>\n");
				fprintf(fr, "          <href>http://earth.google.com/images/kml-icons/track-directional/track-0.png</href>\n");
				fprintf(fr, "        </Icon>\n");
				fprintf(fr, "      </IconStyle>\n");
				fprintf(fr, "      <LineStyle>\n");
				fprintf(fr, "        <color>99ffac59</color>\n");
				fprintf(fr, "        <width>6</width>\n");
				fprintf(fr, "      </LineStyle>\n");
				fprintf(fr, "    </Style>\n");
				fprintf(fr, "    <Style id=\"multiTrack_h\">\n");
				fprintf(fr, "      <IconStyle>\n");
				fprintf(fr, "        <scale>1.2</scale>\n");
				fprintf(fr, "        <Icon>\n");
				fprintf(fr, "          <href>http://earth.google.com/images/kml-icons/track-directional/track-0.png</href>\n");
				fprintf(fr, "        </Icon>\n");
				fprintf(fr, "      </IconStyle>\n");
				fprintf(fr, "      <LineStyle>\n");
				fprintf(fr, "        <color>99ffac59</color>\n");
				fprintf(fr, "        <width>8</width>\n");
				fprintf(fr, "      </LineStyle>\n");
				fprintf(fr, "    </Style>\n");
				fprintf(fr, "    <StyleMap id=\"multiTrack\">\n");
				fprintf(fr, "      <Pair>\n");
				fprintf(fr, "        <key>normal</key>\n");
				fprintf(fr, "        <styleUrl>#multiTrack_n</styleUrl>\n");
				fprintf(fr, "      </Pair>\n");
				fprintf(fr, "      <Pair>\n");
				fprintf(fr, "        <key>highlight</key>\n");
				fprintf(fr, "        <styleUrl>#multiTrack_h</styleUrl>\n");
				fprintf(fr, "      </Pair>\n");
				fprintf(fr, "    </StyleMap>\n");

				fprintf(fr, "    <Schema id=\"schema\">\n");
				fprintf(fr, "      <gx:SimpleArrayField name=\"roll\" type=\"float\">\n");
				fprintf(fr, "        <displayName>Roll</displayName>\n");
				fprintf(fr, "      </gx:SimpleArrayField>\n");
				fprintf(fr, "      <gx:SimpleArrayField name=\"pitch\" type=\"float\">\n");
				fprintf(fr, "        <displayName>Pitch</displayName>\n");
				fprintf(fr, "      </gx:SimpleArrayField>\n");
				fprintf(fr, "      <gx:SimpleArrayField name=\"yaw\" type=\"float\">\n");
				fprintf(fr, "        <displayName>Yaw</displayName>\n");
				fprintf(fr, "      </gx:SimpleArrayField>\n");
				fprintf(fr, "      <gx:SimpleArrayField name=\"alt\" type=\"float\">\n");
				fprintf(fr, "        <displayName>Alt</displayName>\n");
				fprintf(fr, "      </gx:SimpleArrayField>\n");
				fprintf(fr, "      <gx:SimpleArrayField name=\"speed\" type=\"float\">\n");
				fprintf(fr, "        <displayName>Speed</displayName>\n");
				fprintf(fr, "      </gx:SimpleArrayField>\n");
				fprintf(fr, "      <gx:SimpleArrayField name=\"gpsfix\" type=\"int\">\n");
				fprintf(fr, "        <displayName>GPS-Fix</displayName>\n");
				fprintf(fr, "      </gx:SimpleArrayField>\n");
				fprintf(fr, "      <gx:SimpleArrayField name=\"numSat\" type=\"int\">\n");
				fprintf(fr, "        <displayName>Sats</displayName>\n");
				fprintf(fr, "      </gx:SimpleArrayField>\n");
				fprintf(fr, "    </Schema>\n");


				fprintf(fr, "    <Folder>\n");
				fprintf(fr, "      <name>Tracks</name>\n");
				fprintf(fr, "      <Placemark>\n");
				fprintf(fr, "        <name>2010-05-28T01:16:35.000Z</name>\n");
				fprintf(fr, "        <styleUrl>#multiTrack</styleUrl>\n");
				fprintf(fr, "        <gx:Track>\n");

				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);
						struct tm strukt;
						time_t liczba_sekund = (time_t)(lsec);
						localtime_r(&liczba_sekund, &strukt); 
						fprintf(fr, "<when>%02d-%02d-%dT%02d:%02d:%02dZ</when>\n", strukt.tm_year + 1900, strukt.tm_mon + 1, strukt.tm_mday, strukt.tm_hour, strukt.tm_min, strukt.tm_sec);
						point_nr++;
					}
				}

				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);
						fprintf(fr, "<gx:coord>%f, %f, %f</gx:coord>\n", p_long, p_lat, p_alt);
						point_nr++;
					}
				}


				fprintf(fr, "          <ExtendedData>\n");
				fprintf(fr, "            <SchemaData schemaUrl=\"#schema\">\n");

				fprintf(fr, "              <gx:SimpleArrayData name=\"pitch\">\n");
				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);
						fprintf(fr, "<gx:value>%f</gx:value>\n", pitch);
						point_nr++;
					}
				}
				fprintf(fr, "              </gx:SimpleArrayData>\n");

				fprintf(fr, "              <gx:SimpleArrayData name=\"roll\">\n");
				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);
						fprintf(fr, "<gx:value>%f</gx:value>\n", roll);
						point_nr++;
					}
				}
				fprintf(fr, "              </gx:SimpleArrayData>\n");

				fprintf(fr, "              <gx:SimpleArrayData name=\"yaw\">\n");
				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);
						fprintf(fr, "<gx:value>%f</gx:value>\n", yaw);
						point_nr++;
					}
				}
				fprintf(fr, "              </gx:SimpleArrayData>\n");


				fprintf(fr, "              <gx:SimpleArrayData name=\"alt\">\n");
				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw);
						fprintf(fr, "<gx:value>%f</gx:value>\n", p_alt);
						point_nr++;
					}
				}
				fprintf(fr, "              </gx:SimpleArrayData>\n");


				fprintf(fr, "              <gx:SimpleArrayData name=\"speed\">\n");
				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw, &speed, &gpsfix, &numSat);
						fprintf(fr, "<gx:value>%f</gx:value>\n", speed);
						point_nr++;
					}
				}
				fprintf(fr, "              </gx:SimpleArrayData>\n");


				fprintf(fr, "              <gx:SimpleArrayData name=\"gpsfix\">\n");
				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw, &speed, &gpsfix, &numSat);
						fprintf(fr, "<gx:value>%i</gx:value>\n", gpsfix);
						point_nr++;
					}
				}
				fprintf(fr, "              </gx:SimpleArrayData>\n");

				fprintf(fr, "              <gx:SimpleArrayData name=\"numSat\">\n");
				point_nr = 0;
				fseek(log_fr, 0, SEEK_SET);
				while (fgets(line, 500, log_fr) != NULL) {
					if (strncmp(line, "GPS;", 4) == 0) {
						sscanf(line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &p_lat, &p_long, &p_alt);
					} else if (strncmp(line, "ATT;", 4) == 0) {
						sscanf(line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &pitch, &roll, &yaw, &speed, &gpsfix, &numSat);
						fprintf(fr, "<gx:value>%i</gx:value>\n", numSat);
						point_nr++;
					}
				}
				fprintf(fr, "              </gx:SimpleArrayData>\n");

				fprintf(fr, "            </SchemaData>\n");
				fprintf(fr, "          </ExtendedData>\n");

				fprintf(fr, "        </gx:Track>\n");
				fprintf(fr, "      </Placemark>\n");
				fprintf(fr, "    </Folder>\n");
*/


				fprintf(fr, "  </Document>\n");
				fprintf(fr, "</kml>\n");
			        fclose(fr);
			}
			fclose(log_fr);
		}
	}
}

uint8_t logplay_cmd_kml (char *name, float x, float y, int8_t button, float data) {
	logplay_export_kml(logplay_file, "/tmp/export.kml");
	system("googleearth /tmp/export.kml &");
	return 0;
}

uint8_t logplay_cmd_pause (char *name, float x, float y, int8_t button, float data) {
	logplay_pause = 1 - logplay_pause;
	return 0;
}

uint8_t logplay_cmd_step (char *name, float x, float y, int8_t button, float data) {
	logplay_pause = 1;
	logplay_msec += (uint32_t)data;
	if (data < 0.0) {
		logplay_play = 2;
	}
	return 0;
}

uint8_t logplay_cmd_play (char *name, float x, float y, int8_t button, float data) {
	logplay_play = 1 - logplay_play;
	logplay_pause = 0;
	logplay_msec = 0;
	return 0;
}

uint8_t logplay_cmd_open (char *name, float x, float y, int8_t button, float data) {
	logplay_list = 1 - logplay_list;
	logplay_filelist = 0;
	logplay_play = 0;
	logplay_pause = 0;
	logplay_msec = 0;
	return 0;
}

uint8_t logplay_cmd_open_ok (char *name, float x, float y, int8_t button, float data) {
	printf("## open logfile: %s ##\n", name);
	strcpy(logplay_file, name);
	logplay_list = 0;
	logplay_filelist = 0;
	return 0;
}

uint8_t logplay_cmd_play_updown (char *name, float x, float y, int8_t button, float data) {
	if (data < 0.0 && logplay_filelist == 0) {
		return 0;
	}
	logplay_filelist += (int)data;
	return 0;
}

void logplay_draw_control (ESContext *esContext, float x1, float y1) {
	float w = 1.2;
	float h = 0.2;
	float x = x1 - w / 2.0;
	float y = y1 - h / 2.0;
	float max = 7.0;
	char tmp_str[20];

	draw_box_f3(esContext, x, y, 0.002, x + w, y + h, 0.002, 0, 0, 0, 127);
	draw_box_f3(esContext, x + 0.01, y + 0.01, 0.002, x + w - 0.01, y + h / 2 - 0.01, 0.002, 0, 0, 0, 127);
	draw_image_button(esContext, "rew", view_mode, TEXTURE_PLAYER_REW    , x + w / max * 1, y + h / 4 * 3, 0.003, h / 2, h / 2, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_step, -100.0);
	if (logplay_play == 1) {
		draw_image_button(esContext, "stop", view_mode, TEXTURE_PLAYER_STOP  , x + w / max * 2, y + h / 4 * 3, 0.003, h / 2, h / 2, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_play, 0.0);
	} else {
		draw_image_button(esContext, "play", view_mode, TEXTURE_PLAYER_PLAY  , x + w / max * 2, y + h / 4 * 3, 0.003, h / 2, h / 2, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_play, 0.0);
	}
	draw_image_button(esContext, "pause", view_mode, TEXTURE_PLAYER_PAUSE, x + w / max * 3, y + h / 4 * 3, 0.003, h / 2, h / 2, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_pause, 0.0);
	draw_image_button(esContext, "ffw", view_mode, TEXTURE_PLAYER_FFW    , x + w / max * 4, y + h / 4 * 3, 0.003, h / 2, h / 2, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_step, 100.0);
	draw_image_button(esContext, "open", view_mode, TEXTURE_PLAYER_OPEN  , x + w / max * 5, y + h / 4 * 3, 0.003, h / 2, h / 2, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_open, 0.0);
	draw_image_button(esContext, "kml", view_mode, TEXTURE_PLAYER_KML  , x + w / max * 6, y + h / 4 * 3, 0.003, h / 2, h / 2, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_kml, 0.0);

	struct tm strukt;
	time_t liczba_sekund = (time_t)(logplay_msec / 1000 + logplay_startsec);
	localtime_r(&liczba_sekund, &strukt); 
//	sprintf(tmp_str, "%04i.%03is", logplay_msec / 1000, logplay_msec % 1000);
	sprintf(tmp_str, "%02d.%02d.%d %02d:%02d:%02d.%03i", strukt.tm_mday, strukt.tm_mon + 1, strukt.tm_year + 1900, strukt.tm_hour, strukt.tm_min, strukt.tm_sec, logplay_msec % 1000);
	draw_button(esContext, "timer", view_mode, tmp_str, FONT_GREEN, x + w / 2, y + h / 4 * 1, 0.003, 0.06, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_play, 0.0);
	if (logplay_list == 1) {
		draw_box_f3(esContext, x, -0.8, 0.002, x + w, y, 0.002, 0, 0, 0, 200);
		DIR *dir = NULL;
		struct dirent *dir_entry = NULL;
		struct stat statbuf;
		char new_path[400];
		char directory[400];
		int n = 0;
		int n2 = 0;
		draw_button(esContext, "up", view_mode, "[^]", FONT_GREEN, x + w - 0.1, -0.8 + 0.05, 0.001, 0.06, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_play_updown, -1.0);
		draw_button(esContext, "down", view_mode, "[v]", FONT_GREEN, x + w - 0.1, y - 0.05, 0.001, 0.06, ALIGN_CENTER, ALIGN_CENTER, logplay_cmd_play_updown, 1.0);
		sprintf(directory, "%s/logs", BASE_DIR);
		if ((dir = opendir(directory)) != NULL) {
			while ((dir_entry = readdir(dir)) != 0) {
				if (dir_entry->d_name[1] != '.') {
					sprintf(new_path, "%s/%s", directory, dir_entry->d_name);
					if (lstat(new_path, &statbuf) == 0) {
						if (statbuf.st_mode&S_IFDIR) {
						} else {
							if (logplay_filelist == n2) {
								time_t liczba_sekund = (time_t)(atoi(dir_entry->d_name));
								localtime_r(&liczba_sekund, &strukt);
								FILE *f;
								int pos;
								int end;
								f = fopen(new_path, "r");
								pos = ftell(f);
								fseek(f, 0, SEEK_END);
								end = ftell(f);
								fclose(f);
								if ((end - pos) < 1024) {
									sprintf(tmp_str, "%02d.%02d.%d %02d:%02d:%02d (%iB)", strukt.tm_mday, strukt.tm_mon + 1, strukt.tm_year + 1900, strukt.tm_hour, strukt.tm_min, strukt.tm_sec, (end - pos));
								} else {
									sprintf(tmp_str, "%02d.%02d.%d %02d:%02d:%02d (%iKB)", strukt.tm_mday, strukt.tm_mon + 1, strukt.tm_year + 1900, strukt.tm_hour, strukt.tm_min, strukt.tm_sec, (end - pos) / 1024);
								}
								draw_button(esContext, new_path, view_mode, tmp_str, FONT_WHITE, x + 0.05, -0.8 + n * 0.1 + 0.15, 0.002, 0.06, 0, 0, logplay_cmd_open_ok, 0.0);
							}
							n++;
						}
					}
				}
				if (n > 10) {
					n2++;
					n = 0;
				}
			}
			closedir(dir);
			dir = NULL;
		}
	}
}

void LogSave (void) {
	char file[1024];
	sprintf(file, "%s/logs/%i.log", BASE_DIR, log_startup_time);
	if (Logdata != NULL) {
		if (strlen(Logdata) > 200) {
		        FILE *fr;
		        fr = fopen(file, "w");
			if (fr != 0) {
				fprintf(fr, Logdata);
			        fclose(fr);
			}
		}
		free(Logdata);
		Logdata = NULL;
	}
}

void LogInit (void) {
	log_startup_time = time(0);
	if (Logdata != NULL) {
		free(Logdata);
		Logdata = NULL;
	}
	if (Logdata == NULL) {
		Logdata = malloc(1);
		Logdata[0] = 0;
	}
}

void LogAppend (char *line) {

	if (Logdata != NULL) {
		Logdata = realloc(Logdata, (int)(strlen(Logdata) + strlen(line) + 2));
		strcat(Logdata, line);
		strcat(Logdata, "\n");
	}
}

void Logging (void) {
	if (logmode == LOGGING_PLAY) {
		char line[512];
		static char last_line[512];
		static FILE *fr = NULL;
		if (logplay_play == 2 && logplay_open == 1) {
			logplay_play = 1;
			logplay_open = 0;
			fclose(fr);
			fr = NULL;
		}
		if (logplay_play == 0 && logplay_open == 1) {
			logplay_open = 0;
			fclose(fr);
			fr = NULL;
		}
		if (logplay_play == 1) {
			if (logplay_open == 0 && logplay_file[0] != 0) {
				logplay_open = 1;
				logplay_startsec = 0;
				fr = fopen(logplay_file, "r");
				if (fgets(line, 500, fr) != NULL) {
					strcpy(last_line, line);
				} else {
					logplay_open = 0;
					logplay_play = 0;
					fclose(fr);
					fr = NULL;
				}
			}
			struct timeval tv;
			gettimeofday(&tv, NULL);  
			uint32_t sec = tv.tv_sec;
			uint32_t micros = tv.tv_usec / 1000;
			uint32_t lsec = 0;
			uint32_t lmicros = 0;
			if (fr != NULL) {
				while (1 == 1) {
					if (last_line[3] == ';') {
						sscanf(last_line + 4, "%i.%i;", &lsec, &lmicros);
						if (logplay_startsec == 0) {
							logplay_startsec = lsec;
						}
					} else {
						if (fgets(line, 500, fr) != NULL) {
							strcpy(last_line, line);
							printf("NEXT_LINE: %s\n", line);
							continue;
						} else {
							printf("######## file end ########\n");
							logplay_open = 0;
							logplay_play = 0;
							fclose(fr);
							fr = NULL;
							break;
						}
					}
					if (logplay_msec >= (lsec - logplay_startsec) * 1000 + lmicros) {
						if (strncmp(last_line, "GPS;", 4) == 0) {
							sscanf(last_line, "GPS;%i.%i;%f;%f;%f", &lsec, &lmicros, &ModelData.p_lat, &ModelData.p_long, &ModelData.p_alt);
						} else if (strncmp(last_line, "ATT;", 4) == 0) {
							sscanf(last_line, "ATT;%i.%i;%f;%f;%f", &lsec, &lmicros, &ModelData.pitch, &ModelData.roll, &ModelData.yaw);
						}
						if (fgets(line, 500, fr) != NULL) {
							strcpy(last_line, line);
							printf("NEXT_LINE: %s\n", line);
							continue;
						} else {
							printf("######## file end ########\n");
							logplay_open = 0;
							logplay_play = 0;
							fclose(fr);
							fr = NULL;
							break;
						}
					} else {
						break;
					}
				}
			}
			if (logplay_pause == 0) {
				logplay_msec += 40;
			}
		}
	} else if (logmode == LOGGING_ON) {
		char line[512];
		static float last_lat = 0.0;
		static float last_lon = 0.0;
		static float last_alt = 0.0;
		static int8_t last_sats = 0;
		static float last_pitch = 0.0;
		static float last_roll = 0.0;
		static float last_yaw = 0.0;
		static int16_t last_radio[8];
		static float last_ampere = 0.0;
		static float last_voltage = 0.0;
		static uint8_t last_mode = 0;
		static uint8_t last_armed = 0;
		static uint8_t last_status = 0;
		static uint8_t last_modeltype = 0;
		static uint8_t last_teletype = 0;
		static uint8_t last_heartbeat = 0;
		uint8_t n= 0;
		uint8_t logflag = 0;

		struct timeval tv;
		gettimeofday(&tv, NULL);  
		uint32_t sec = tv.tv_sec;
		uint32_t micros = tv.tv_usec / 1000;

		if (last_lat != ModelData.p_lat || last_lon != ModelData.p_long || (int)last_alt != (int)ModelData.p_alt) {
			sprintf(line, "GPS;%i.%03i;%f;%f;%f;%f;%i;%i", sec, micros, ModelData.p_lat, ModelData.p_long, ModelData.p_alt, ModelData.speed, ModelData.gpsfix, ModelData.numSat);
			LogAppend(line);
			last_lat = ModelData.p_lat;
			last_lon = ModelData.p_long;
			last_alt = ModelData.p_alt;
			last_sats = ModelData.numSat;
		}
		if ((int)last_pitch != (int)ModelData.pitch || (int)last_roll != (int)ModelData.roll || (int)last_yaw != (int)ModelData.yaw) {
			sprintf(line, "ATT;%i.%03i;%f;%f;%f", sec, micros, ModelData.pitch, ModelData.roll, ModelData.yaw);
			LogAppend(line);
			last_pitch = ModelData.pitch;
			last_roll = ModelData.roll;
			last_yaw = ModelData.yaw;
		}
		logflag = 0;
		for (n = 0; n < 8; n++) {
			if (last_radio[n] != ModelData.radio[n]) {
				logflag = 1;
				last_radio[n] = ModelData.radio[n];
			}
		}
		if (logflag == 1) {
			logflag = 0;
			sprintf(line, "RC0;%i.%03i;%i;%i;%i;%i;%i;%i;%i;%i", sec, micros, ModelData.radio[0], ModelData.radio[1], ModelData.radio[2], ModelData.radio[3], ModelData.radio[4], ModelData.radio[5], ModelData.radio[6], ModelData.radio[7]);
			LogAppend(line);
		}
		if (last_voltage != ModelData.voltage || last_voltage != ModelData.voltage) {
			sprintf(line, "AV0;%i.%03i;%f;%f;%f;%f;%f;%f;%f;%f;%i;%i", sec, micros, ModelData.ampere, ModelData.voltage, ModelData.voltage_zell[0], ModelData.voltage_zell[1], ModelData.voltage_zell[2], ModelData.voltage_zell[3], ModelData.voltage_zell[4], ModelData.voltage_zell[5], ModelData.temperature[0], ModelData.temperature[1]);
			LogAppend(line);
			last_ampere = ModelData.ampere;
			last_voltage = ModelData.voltage;
		}
		if (last_mode != ModelData.mode || last_armed != ModelData.armed || last_status != ModelData.status) {
			sprintf(line, "AM0;%i.%03i;%i;%i;%i", sec, micros, ModelData.mode, ModelData.armed, ModelData.status);
			LogAppend(line);
			last_mode = ModelData.mode;
			last_armed = ModelData.armed;
			last_status = ModelData.status;
		}
		if (last_modeltype != ModelData.modeltype || last_teletype != ModelData.teletype) {
			sprintf(line, "MT0;%i.%03i;%i;%i", sec, micros, ModelData.modeltype, ModelData.teletype);
			LogAppend(line);
			last_modeltype = ModelData.modeltype;
			last_teletype = ModelData.teletype;
		}
		if (last_heartbeat == 0 && ModelData.heartbeat > 0) {
			sprintf(line, "HB0;%i.%03i;%i", sec, micros, ModelData.heartbeat);
			LogAppend(line);
			last_heartbeat = 1;
		} if (last_heartbeat != 0 && ModelData.heartbeat == 0) {
			sprintf(line, "HB0;%i.%03i;0", micros);
			LogAppend(line);
			last_heartbeat = 0;
		}
	}
}