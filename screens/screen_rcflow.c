
#include <all.h>


#ifdef RPI_NO_X
#define SIMPLE_DRAW	1
#endif

#define SCALE_MIN	0.1
#define SCALE_MAX	6.0
#define LINK_SPACE	0.075
#define LINK_WIDTH	0.15
#define CONN_RADIUS	0.025
#define TITLE_HEIGHT	0.05
#define PLUGIN_WIDTH 	0.3
#define SLIDER_WIDTH 	0.22

#define MAX_INPUTS	12
#define MAX_OUTPUTS	12
#define MAX_PLUGINS	64
#define MAX_LINKS	64
#define MAX_NAMELEN	16

#define MAX_CURVE_POINTS 9

#define MAX_UNDO	8


#define PLUGIN_X1 ((RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) + canvas_x) * scale)
#define PLUGIN_Y1 ((RcPlugin[id].y - num_max / 2 * LINK_SPACE - TITLE_HEIGHT + canvas_y) * scale)
#define PLUGIN_X2 ((RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + canvas_x) * scale)
#define PLUGIN_Y2 ((RcPlugin[id].y + num_max / 2 * LINK_SPACE + canvas_y) * scale)

#define ADC_TEST 1

static int16_t edit_title = -1;
static uint8_t startup = 0;
static uint8_t unsave = 0;
static uint8_t lock_timer = 0;
static uint8_t lock = 0;
static uint8_t undo = 0;
static uint8_t undo_timer = 0;
static uint8_t select_plugin = 0;
static uint8_t del_plugin = 0;
static char link_select_from[100];
static char link_select_to[100];
static float canvas_x = 0.0;
static float canvas_y = 0.0;
static uint8_t virt_view = 0;
static char setup_name[100];

static float scale = 0.8;



//static Tcl_Interp *rcflow_tcl_interp;
static uint8_t rcflow_tcl_startup = 0;

enum {
	RCFLOW_INOUT_TYPE_BILINEAR,
	RCFLOW_INOUT_TYPE_ONOFF,
	RCFLOW_INOUT_TYPE_LINEAR,
	RCFLOW_INOUT_TYPE_CURVE,
	RCFLOW_INOUT_TYPE_SWITCH,
	RCFLOW_INOUT_TYPE_TEXT,
	RCFLOW_INOUT_TYPE_TEMP,
};

enum {
	RCFLOW_PLUGIN_INPUT,
	RCFLOW_PLUGIN_OUTPUT,
	RCFLOW_PLUGIN_ATV,
	RCFLOW_PLUGIN_LIMIT,
	RCFLOW_PLUGIN_MIXER,
	RCFLOW_PLUGIN_CURVE,
	RCFLOW_PLUGIN_EXPO,
	RCFLOW_PLUGIN_DELAY,
	RCFLOW_PLUGIN_ADC,
	RCFLOW_PLUGIN_SW,
	RCFLOW_PLUGIN_ENC,
	RCFLOW_PLUGIN_PPM,
	RCFLOW_PLUGIN_H120,
	RCFLOW_PLUGIN_SWITCH,
	RCFLOW_PLUGIN_RANGESW,
	RCFLOW_PLUGIN_TIMER,
	RCFLOW_PLUGIN_AND,
	RCFLOW_PLUGIN_OR,
	RCFLOW_PLUGIN_PULSE,
	RCFLOW_PLUGIN_SINUS,
	RCFLOW_PLUGIN_ALARM,
	RCFLOW_PLUGIN_VADC,
	RCFLOW_PLUGIN_VSW,
	RCFLOW_PLUGIN_TCL,
	RCFLOW_PLUGIN_BUTTERFLY,
	RCFLOW_PLUGIN_MULTIPLEX_IN,
	RCFLOW_PLUGIN_MULTIPLEX_OUT,
	RCFLOW_PLUGIN_MULTIVALUE,
	RCFLOW_PLUGIN_LAST,
};

typedef struct {
	char name[MAX_NAMELEN];
	int16_t value;
	uint8_t type;
	uint8_t option;
	float tmp_value;
	char tmp_text[100];
} RcFlowSignal;

typedef struct {
	char title[16];
	char name[MAX_NAMELEN];
	uint8_t type;
	uint8_t vview;
	uint16_t option;
	float x;
	float y;
	float x_virt;
	float y_virt;
	uint8_t open;
	char text[1024];
	RcFlowSignal input[MAX_INPUTS];
	RcFlowSignal output[MAX_OUTPUTS];
} RcFlowPlugin;

typedef struct {
	char from[MAX_NAMELEN * 2];
	char to[MAX_NAMELEN * 2];
} RcFlowLink;

static RcFlowPlugin RcPlugin[MAX_PLUGINS];
static RcFlowLink RcLink[MAX_LINKS];

typedef struct {
	int16_t value;
	uint8_t type;
	uint8_t option;
} RcFlowSignalEmbedded;

typedef struct {
	uint8_t type;
	uint8_t pad1;
	uint16_t option;
	RcFlowSignalEmbedded input[MAX_INPUTS];
	RcFlowSignalEmbedded output[MAX_OUTPUTS];
} RcFlowPluginEmbedded;

typedef struct {
	uint8_t from[2];
	uint8_t pad1;
	uint8_t pad2;
	uint8_t to[2];
	uint8_t pad3;
	uint8_t pad4;
} RcFlowLinkEmbedded;

static RcFlowPluginEmbedded RcPluginEmbedded[MAX_PLUGINS];
static RcFlowLinkEmbedded RcLinkEmbedded[MAX_LINKS];

int16_t rcflow_value_limit (int16_t value, int16_t min, int16_t max);

static char plugintype_names[RCFLOW_PLUGIN_LAST][11] = {
	"INPUT",
	"OUTPUT",
	"ATV",
	"LIMIT",
	"MIXER",
	"CURVE",
	"EXPO",
	"DELAY",
	"ADC",
	"SW",
	"ENC",
	"PPM",
	"H120",
	"SWITCH",
	"RANGESW",
	"TIMER",
	"AND",
	"OR",
	"PULSE",
	"SINUS",
	"ALARM",
	"VADC",
	"VSW",
	"TCL",
};


int rcflow_fd = -1;

#ifdef ADC_TEST

int adc[MAX_OUTPUTS];
int sw[MAX_OUTPUTS];
int out[MAX_OUTPUTS];
char buf[200];
char buffer[200];

#endif

int rcflow_plugin_move_id = -1;
int rcflow_plugin_move_timer = 0;

static int16_t menu_set = 0;
static int16_t menu_set_reverse = 0;
static int16_t menu_set_type = 0;
static int16_t menu_set_plugin = 0;
static int16_t menu_set_input = 0;


uint8_t rctransmitter_get_type_by_name (char *name) {
	if (strcmp(name, "MULTIWII_21") == 0) {
		return TELETYPE_MULTIWII_21;
	} else if (strcmp(name, "AUTOQUAD") == 0) {
		return TELETYPE_AUTOQUAD;
	} else if (strcmp(name, "ARDUPILOT") == 0) {
		return TELETYPE_ARDUPILOT;
	} else if (strcmp(name, "MEGAPIRATE_NG") == 0) {
		return TELETYPE_MEGAPIRATE_NG;
	} else if (strcmp(name, "OPENPILOT") == 0) {
		return TELETYPE_OPENPILOT;
	} else if (strcmp(name, "GPS_NMEA") == 0) {
		return TELETYPE_GPS_NMEA;
	} else if (strcmp(name, "FRSKY") == 0) {
		return TELETYPE_FRSKY;
	} else if (strcmp(name, "BASEFLIGHT") == 0) {
		return TELETYPE_BASEFLIGHT;
	} else {
		return 255;
	}
	return 0;
}

/*
static int rcflow_tcl_output_Cmd (ClientData cdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
	int n1 = 0;
	int n2 = 0;
	int n3 = 0;
	Tcl_Obj *res;
	if (objc != 4) {
		Tcl_WrongNumArgs(interp, 1, objv, "plugin output value");
		return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[1], &n1) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[2], &n2) != TCL_OK) {
		return TCL_ERROR;
	}
	if (Tcl_GetIntFromObj(interp, objv[3], &n3) != TCL_OK) {
		return TCL_ERROR;
	}
	if (n1 >= MAX_PLUGINS) {
		SDL_Log("rcflow: TCL ERROR: plugin out of range %i (max %i)", n1, MAX_PLUGINS);
		return TCL_ERROR;
	}
	if (n2 >= MAX_OUTPUTS) {
		SDL_Log("rcflow: TCL ERROR: output out of range %i (max %i)", n2, MAX_OUTPUTS);
		return TCL_ERROR;
	}
	RcPluginEmbedded[n1].output[n2].value = rcflow_value_limit(n3, -1000, 1000);
	res = Tcl_NewLongObj(n3);
	Tcl_SetObjResult(interp, res);
	return TCL_OK;
}
*/

static void rcflow_tcl_init (void) {
	rcflow_tcl_startup = 1;
/*
	SDL_Log("rcflow: init RcFlow-TCL...\n");
	rcflow_tcl_interp = Tcl_CreateInterp();
	if (TCL_OK != Tcl_Init(rcflow_tcl_interp)) {
		SDL_Log("rcflow: ...failed (%s)\n", Tcl_GetStringResult(rcflow_tcl_interp));
		return;
	}
//	Tcl_CreateObjCommand(rcflow_tcl_interp, "ModelData[ModelActive]", ModelData[ModelActive]_Cmd, NULL, NULL);
	Tcl_CreateObjCommand(rcflow_tcl_interp, "set_output", rcflow_tcl_output_Cmd, NULL, NULL);
	SDL_Log("...done\n");
*/
	return;
}

/*
static void rcflow_tcl_run (char *script) {
	if (rcflow_tcl_startup == 0) {
		rcflow_tcl_init();
	}
	tcl_update_modeldata();
	if (Tcl_Eval(rcflow_tcl_interp, script) != TCL_OK) {
		SDL_Log("rcflow: TCL-ERROR:\n");
		SDL_Log("rcflow: #######################################################\n");
		SDL_Log("%s\n", script);
		SDL_Log("rcflow: #######################################################\n");
		SDL_Log("%s\n", Tcl_GetStringResult(rcflow_tcl_interp));
		SDL_Log("rcflow: #######################################################\n");
	}
}
*/

static void die(char *msg) {
	SDL_Log("rcflow: %s", msg);
	return;
}

void rcflow_parseInput (xmlDocPtr doc, xmlNodePtr cur, uint8_t plugin, uint8_t input) { 
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcPlugin[plugin].input[input].name, (char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"value"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].input[input].value = atoi((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"type"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].input[input].type = atoi((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"option"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].input[input].option = atoi((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"tmp_value"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].input[input].tmp_value = atof((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"tmp_text"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcPlugin[plugin].input[input].tmp_text, (char *)key);
				xmlFree(key);
			}
		}
		cur = cur->next;
	}
	return;
}

void rcflow_parseOutput (xmlDocPtr doc, xmlNodePtr cur, uint8_t plugin, uint8_t output) { 
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcPlugin[plugin].output[output].name, (char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"value"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].output[output].value = atoi((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"type"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].output[output].type = atoi((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"option"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].output[output].option = atoi((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"tmp_value"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				RcPlugin[plugin].output[output].tmp_value = atof((char *)key);
				xmlFree(key);
			}
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"tmp_text"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcPlugin[plugin].output[output].tmp_text, (char *)key);
				xmlFree(key);
			}
		}
		cur = cur->next;
	}
	return;
}

void rcflow_parsePlugin (xmlDocPtr doc, xmlNodePtr cur, uint8_t plugin) { 
	uint8_t input = 0;
	uint8_t output = 0;
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			strcpy(RcPlugin[plugin].name, (char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"title"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcPlugin[plugin].title, (char *)key);
			}
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"text"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcPlugin[plugin].text, (char *)key);
			}
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"type"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].type = atoi((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"vview"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].vview = atoi((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"option"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].option = atoi((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"open"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].open = atoi((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"x"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].x = atof((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"y"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].y = atof((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"x_virt"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].x_virt = atof((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"y_virt"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			RcPlugin[plugin].y_virt = atof((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"input"))) {
			rcflow_parseInput(doc, cur, plugin, input++);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"output"))) {
			rcflow_parseOutput(doc, cur, plugin, output++);
		}
		cur = cur->next;
	}
	return;
}

void rcflow_parseLink (xmlDocPtr doc, xmlNodePtr cur, uint8_t link) { 
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"from"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcLink[link].from, (char *)key);
			}
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"to"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(RcLink[link].to, (char *)key);
			}
			xmlFree(key);
		}
		cur = cur->next;
	}
	return;
}

void rcflow_parseTelemetry (xmlDocPtr doc, xmlNodePtr cur) { 
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"type"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				ModelData[ModelActive].teletype = rctransmitter_get_type_by_name((char *)key);
			}
			xmlFree(key);
		}
		cur = cur->next;
	}
	return;
}

void rcflow_parseMavlinkParam (xmlDocPtr doc, xmlNodePtr cur, uint16_t param) { 
	xmlChar *key;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(MavLinkVars[ModelActive][param].name, (char *)key);
			}
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"value"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				MavLinkVars[ModelActive][param].value = atof((char *)key);
			}
			xmlFree(key);
		}
		cur = cur->next;
	}
	return;
}

void rcflow_parseMavlink (xmlDocPtr doc, xmlNodePtr cur) { 
	uint16_t param = 0;
	for (param = 0; param < MAVLINK_PARAMETER_MAX; param++) {
		MavLinkVars[ModelActive][param].name[0] = 0;
		MavLinkVars[ModelActive][param].value = 0.0;
	}
	param = 0;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"param"))) {
			rcflow_parseMavlinkParam (doc, cur, param++);
		}
		cur = cur->next;
	}
	return;
}

static void rcflow_parseDoc (char *docname) {
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	uint8_t link = 0;
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar *key;

	if (strncmp(docname, "./", 2) == 0) {
		docname += 2;
	}

	char *buffer = NULL;
	int len = 0;
	SDL_RWops *ops_file = SDL_RWFromFile(docname, "r");
	if (ops_file == NULL) {
		SDL_Log("map: Document open failed: %s\n", docname);
		return;
	}
	len = SDL_RWseek(ops_file, 0, SEEK_END);
	SDL_RWseek(ops_file, 0, SEEK_SET);
	buffer = malloc(len);
	SDL_RWread(ops_file, buffer, 1, len);
	doc = xmlParseMemory(buffer, len);
	SDL_RWclose(ops_file);
	free(buffer);

	if (doc == NULL) {
		SDL_Log("rcflow: Document parsing failed: %s\n", docname);
		return;
	}
	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		xmlFreeDoc(doc);
		die("Document is Empty!!!\n");
		return;
	}
	strcpy(setup_name, basename(docname));
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		RcPlugin[plugin].name[0] = 0;
		RcPlugin[plugin].type = 255;
		RcPlugin[plugin].vview = 0;
		for (input = 0; input < MAX_INPUTS; input++) {
			RcPlugin[plugin].input[input].name[0] = 0;
			RcPlugin[plugin].input[input].value = 0;
			RcPlugin[plugin].input[input].type = RCFLOW_INOUT_TYPE_BILINEAR;
			RcPlugin[plugin].input[input].tmp_value = 0.0;
			RcPlugin[plugin].input[input].tmp_text[0] = 0;
		}
		for (output = 0; output < MAX_INPUTS; output++) {
			RcPlugin[plugin].output[output].name[0] = 0;
			RcPlugin[plugin].output[output].value = 0;
			RcPlugin[plugin].output[output].type = RCFLOW_INOUT_TYPE_BILINEAR;
			RcPlugin[plugin].output[output].tmp_value = 0.0;
			RcPlugin[plugin].output[output].tmp_text[0] = 0;
		}
	}
	for (link = 0; link < MAX_LINKS; link++) {
		RcLink[link].from[0] = 0;
		RcLink[link].to[0] = 0;
	}
	plugin = 0;
	input = 0;
	output = 0;
	link = 0;
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"name"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(setup_name, (char *)key);
				if (strstr(setup_name, ".xml\0") <= 0) {
					strcat(setup_name, ".xml");
				}
			}
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"image"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			if ((char *)key != NULL) {
				strcpy(ModelData[ModelActive].image, (char *)key);
			}
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"scale"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			scale = atof((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"virt_view"))) {
			key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
			virt_view = atoi((char *)key);
			xmlFree(key);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"plugin"))) {
			rcflow_parsePlugin (doc, cur, plugin++);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"link"))) {
			rcflow_parseLink (doc, cur, link++);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"telemetry"))) {
//			rcflow_parseTelemetry (doc, cur);
		} else if ((!xmlStrcasecmp(cur->name, (const xmlChar *)"mavlink"))) {
			rcflow_parseMavlink (doc, cur);
		}
		cur = cur->next;
	}
	xmlFreeDoc(doc);
	return;
}

void rcflow_convert_to_Embedded (void) {
	char tmp_str[64];
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	uint8_t link = 0;
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		RcPluginEmbedded[plugin].type = 255;
	}
	for (link = 0; link < MAX_LINKS; link++) {
		RcLinkEmbedded[link].from[0] = 255;
		RcLinkEmbedded[link].from[1] = 255;
		RcLinkEmbedded[link].to[0] = 255;
		RcLinkEmbedded[link].to[1] = 255;
	}
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPlugin[plugin].name[0] != 0) {
			RcPluginEmbedded[plugin].type = RcPlugin[plugin].type;
			RcPluginEmbedded[plugin].option = RcPlugin[plugin].option;
			for (input = 0; input < MAX_INPUTS; input++) {
				RcPluginEmbedded[plugin].input[input].value = RcPlugin[plugin].input[input].value;
				RcPluginEmbedded[plugin].input[input].type = RcPlugin[plugin].input[input].type;
				RcPluginEmbedded[plugin].input[input].option = RcPlugin[plugin].input[input].option;
			}
			for (output = 0; output < MAX_INPUTS; output++) {
				RcPluginEmbedded[plugin].output[output].value = RcPlugin[plugin].output[output].value;
				RcPluginEmbedded[plugin].output[output].type = RcPlugin[plugin].output[output].type;
				RcPluginEmbedded[plugin].output[output].option = RcPlugin[plugin].output[output].option;
			}
		}
	}
	for (link = 0; link < MAX_LINKS; link++) {
		if (RcLink[link].from[0] != 0 && RcLink[link].to[0] != 0) {
			for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
				if (RcPlugin[plugin].name[0] != 0) {
					for (output = 0; output < MAX_OUTPUTS; output++) {
						if (RcPlugin[plugin].output[output].name[0] != 0) {
							sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].output[output].name);
							if (strcmp(RcLink[link].from, tmp_str) == 0) {
								RcLinkEmbedded[link].from[0] = plugin;
								RcLinkEmbedded[link].from[1] = output;
							}
						}
					}
				}
			}
			for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
				if (RcPlugin[plugin].name[0] != 0) {
					for (input = 0; input < MAX_INPUTS; input++) {
						if (RcPlugin[plugin].input[input].name[0] != 0) {
							sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].input[input].name);
							if (strcmp(RcLink[link].to, tmp_str) == 0) {
								RcLinkEmbedded[link].to[0] = plugin;
								RcLinkEmbedded[link].to[1] = input;
							}
						}
					}
				}
			}
		}
	}
}

void rcflow_convert_from_Embedded (void) {
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	uint8_t link = 0;
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPlugin[plugin].name[0] != 0) {
			RcPlugin[plugin].type = RcPluginEmbedded[plugin].type;
			RcPlugin[plugin].option = RcPluginEmbedded[plugin].option;
			for (input = 0; input < MAX_INPUTS; input++) {
				RcPlugin[plugin].input[input].value = RcPluginEmbedded[plugin].input[input].value;
				RcPlugin[plugin].input[input].option = RcPluginEmbedded[plugin].input[input].option;
			}
			for (output = 0; output < MAX_INPUTS; output++) {
				RcPlugin[plugin].output[output].value = RcPluginEmbedded[plugin].output[output].value;
				RcPlugin[plugin].output[output].option = RcPluginEmbedded[plugin].output[output].option;
			}
		}
	}
	for (link = 0; link < MAX_LINKS; link++) {
		if (RcLinkEmbedded[link].from[0] != 255 && RcLinkEmbedded[link].from[1] != 255) {
			if (RcPlugin[RcLinkEmbedded[link].from[0]].type != RCFLOW_PLUGIN_ADC && RcPlugin[RcLinkEmbedded[link].from[0]].type != RCFLOW_PLUGIN_SW && RcPlugin[RcLinkEmbedded[link].from[0]].type != RCFLOW_PLUGIN_ENC) {
				RcPlugin[RcLinkEmbedded[link].from[0]].output[RcLinkEmbedded[link].from[1]].value = RcPluginEmbedded[RcLinkEmbedded[link].from[0]].output[RcLinkEmbedded[link].from[1]].value;
			}
		}
		if (RcLinkEmbedded[link].to[0] != 255 && RcLinkEmbedded[link].to[1] != 255) {
			RcPlugin[RcLinkEmbedded[link].to[0]].input[RcLinkEmbedded[link].to[1]].value = RcPluginEmbedded[RcLinkEmbedded[link].to[0]].input[RcLinkEmbedded[link].to[1]].value;
		}
	}
}

uint8_t rcflow_null (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	return 1;
}

uint8_t rcflow_virt_view (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	if (virt_view < 2) {
		virt_view++;
	} else {
		virt_view = 0;
	}
	return 0;
}

void rcflow_undo_save (void) {
	int16_t n = 0;
	char tmp_str[1024];
	char tmp_str2[1024];
	SDL_Log("rcflow: undo_save \n");
	sprintf(tmp_str, "%s/models", get_datadirectory());
#ifndef WINDOWS
	mkdir(tmp_str, 0755);
#else
	mkdir(tmp_str);
#endif
	for (n = MAX_UNDO - 1; n >= 0; n--) {
		sprintf(tmp_str, "%s/models/rcflow_undo%i.bin", get_datadirectory(), n);
		sprintf(tmp_str2, "%s/models/rcflow_undo%i.bin", get_datadirectory(), n + 1);
		rename(tmp_str, tmp_str2);
	}
        FILE *fr;
	sprintf(tmp_str, "%s/models/rcflow_undo0.bin", get_datadirectory());
        fr = fopen(tmp_str, "wb");
	if (fr != 0) {
		fwrite(&RcPlugin[0], sizeof(RcFlowPlugin) * MAX_PLUGINS, 1, fr);
		fwrite(&RcLink[0], sizeof(RcFlowLink) * MAX_LINKS, 1, fr);
	        fclose(fr);
	}
	undo = 0;
}


uint8_t rcflow_redo (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	char tmp_str[64];
	if (undo > 0) {
		undo--;
	}
	SDL_Log("rcflow: redo %i\n", undo);
        FILE *fr;
	sprintf(tmp_str, "%s/models/rcflow_undo%i.bin", get_datadirectory(), undo);
        fr = fopen(tmp_str, "r");
	if (fr != 0) {
		fread(&RcPlugin[0], sizeof(RcFlowPlugin) * MAX_PLUGINS, 1, fr);
		fread(&RcLink[0], sizeof(RcFlowLink) * MAX_LINKS, 1, fr);
	        fclose(fr);
	}
	return 0;
}

uint8_t rcflow_undo (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	char tmp_str[64];
	if (undo < MAX_UNDO) {
		undo++;
	}
	SDL_Log("rcflow: undo %i\n", undo);
        FILE *fr;
	sprintf(tmp_str, "%s/models/rcflow_undo%i.bin", get_datadirectory(), undo);
        fr = fopen(tmp_str, "r");
	if (fr != 0) {
		fread(&RcPlugin[0], sizeof(RcFlowPlugin) * MAX_PLUGINS, 1, fr);
		fread(&RcLink[0], sizeof(RcFlowLink) * MAX_LINKS, 1, fr);
	        fclose(fr);
	}
	return 0;
}

uint8_t rcflow_menu_set (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	menu_set = (int16_t)data;
	return 0;
}

uint8_t rcflow_menu_set_reverse_dir (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	uint16_t plugin = atoi(name + 2);
	if (name[0] == 'i') {
		RcPlugin[plugin].input[(int)data].option = 1 - RcPlugin[plugin].input[(int)data].option;
	} else if (name[0] == 'o') {
		RcPlugin[plugin].output[(int)data].option = 1 - RcPlugin[plugin].output[(int)data].option;
	}
	rcflow_update_plugin(plugin);
	unsave = 1;
	return 0;
}

uint8_t rcflow_menu_set_reverse (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	menu_set_reverse = (int16_t)data;
	return 0;
}

uint8_t rcflow_menu_set_input (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	menu_set_input = (int16_t)data;
	return 0;
}

uint8_t rcflow_menu_set_plugin (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	menu_set_plugin = (int16_t)data;
	return 0;
}

uint8_t rcflow_menu_set_type (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	menu_set_type = (int16_t)data;
	menu_set_plugin = -1;
	return 0;
}

uint8_t rcflow_test (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	uint16_t plugin = 0;
	uint16_t input = 0;
	uint16_t link = 0;
	uint16_t type = 0;
	uint8_t linked = 0;
	uint8_t plist = 0;
	uint8_t tlist = 0;
	char tmp_str[128];
	for (type = 0; type < RCFLOW_PLUGIN_LAST; type++) {
		tlist = 0;
		for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
			if (RcPlugin[plugin].name[0] != 0 && RcPlugin[plugin].type == type) {
				plist = 0;
				for (input = 0; input < MAX_INPUTS; input++) {
					if (RcPlugin[plugin].input[input].name[0] != 0) {
						linked = 0;
						if (RcPlugin[plugin].type != RCFLOW_PLUGIN_INPUT) {
							for (link = 0; link < MAX_LINKS; link++) {
								if (RcLink[link].from[0] != 0) {
									sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].input[input].name);
									if (strcmp(RcLink[link].to, tmp_str) == 0) {
										linked = 1;
										break;
									}
								}
							}
						}
						if (linked == 0) {
							if (tlist == 0) {
								tlist = 1;
								SDL_Log("rcflow: %s:\n", plugintype_names[RcPlugin[plugin].type]);
							}
							if (plist == 0) {
								plist = 1;
								if (RcPlugin[plugin].title[0] == 0) {
									SDL_Log("rcflow: 	%s\n", RcPlugin[plugin].name);
								} else {
									SDL_Log("rcflow: 	%s\n", RcPlugin[plugin].title);
								}
							}
							SDL_Log("rcflow:     		%s -- %i -- %i\n", RcPlugin[plugin].input[input].name, RcPlugin[plugin].input[input].value, RcPlugin[plugin].input[input].type);
						}
					}
				}
			}
		}
	}
	return 0;
}

uint8_t rcflow_open (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	int id = (int)data;
	RcPlugin[id].open = 1 - RcPlugin[id].open;
	unsave = 1;
	return 0;
}

uint8_t rcflow_update (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
#ifdef ADC_TEST
	uint8_t start[2] = {'A', 'A'};
	uint8_t stop[2] = {'S', 'S'};
	rcflow_convert_to_Embedded();
	write(rcflow_fd, &start, 2);
	write(rcflow_fd, &RcPluginEmbedded, sizeof(RcPluginEmbedded));
	write(rcflow_fd, &RcLinkEmbedded , sizeof(RcLinkEmbedded));
	write(rcflow_fd, &stop, 2);
#endif
	return 0;
}

uint8_t rcflow_update_plugin (int8_t plugin_num) {
#ifdef ADC_TEST
	int16_t n = 0;
	uint8_t start[2] = {'P', 'P'};
	uint8_t stop[2] = {'S', 'S'};
	uint8_t chksum = plugin_num;
	uint8_t *ptr = (uint8_t *)&RcPluginEmbedded[plugin_num];
	rcflow_convert_to_Embedded();
	for (n = 0; n < sizeof(RcFlowPluginEmbedded); n++) {
//		SDL_Log("%i -- %i/%i: ptr = %i (%i)\n", plugin_num, n, sizeof(RcFlowPluginEmbedded), *ptr, chksum);
		chksum ^= *ptr++;
	}
//	SDL_Log("chksum = %i\n", chksum);
	write(rcflow_fd, &start, 2);
	write(rcflow_fd, &plugin_num, sizeof(plugin_num));
	write(rcflow_fd, &RcPluginEmbedded[plugin_num], sizeof(RcFlowPluginEmbedded));
	write(rcflow_fd, &chksum, sizeof(chksum));
	write(rcflow_fd, &stop, 2);
#endif
	return 0;
}

uint8_t rcflow_update_link (int8_t link_num) {
#ifdef ADC_TEST
	int16_t n = 0;
	uint8_t start[2] = {'L', 'L'};
	uint8_t stop[2] = {'S', 'S'};
	uint8_t chksum = link_num;
	uint8_t *ptr = (uint8_t *)&RcLinkEmbedded[link_num];
	rcflow_convert_to_Embedded();
	for (n = 0; n < sizeof(RcFlowLinkEmbedded); n++) {
//		SDL_Log("%i -- %i/%i: ptr = %i (%i)\n", link_num, n, sizeof(RcFlowLinkEmbedded), *ptr, chksum);
		chksum ^= *ptr++;
	}
//	SDL_Log("chksum = %i\n", chksum);
	write(rcflow_fd, &start, 2);
	write(rcflow_fd, &link_num, sizeof(link_num));
	write(rcflow_fd, &RcLinkEmbedded[link_num], sizeof(RcFlowLinkEmbedded));
	write(rcflow_fd, &chksum, sizeof(chksum));
	write(rcflow_fd, &stop, 2);
#endif
	return 0;
}

uint8_t rcflow_load_xml (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	rcflow_parseDoc(name);
	reset_buttons();
	return 0;
}

uint8_t rcflow_load (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	char tmp_str[128];
	sprintf(tmp_str, "%s/models", get_datadirectory());
	filesystem_set_dir(tmp_str);
	filesystem_set_callback(rcflow_load_xml);
	filesystem_reset_filter();
	filesystem_add_filter(".xml\0");
	filesystem_set_mode(setup.view_mode);
	return 0;
}

uint8_t rcflow_lock (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	lock = 1 - lock;
	return 0;
}

uint8_t rcflow_save_xml (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	int16_t n = 0;
	uint8_t plugin = 0;
	uint8_t link = 0;
	uint8_t input = 0;
	uint8_t output = 0;
        FILE *fr;
	char tmp_str[128];

	if (strstr(name, ".xml\0") <= 0) {
		strcat(name, ".xml");
	}
	strcpy(setup_name, name);
	sprintf(tmp_str, "mkdir -p %s/models", get_datadirectory());
	system(tmp_str);
	sprintf(tmp_str, "%s/models/%s", get_datadirectory(), name);
        fr = fopen(tmp_str, "wb");
	if (fr != 0) {
		fprintf(fr, "<rcflow>\n");
		fprintf(fr, " <name>%s</name>\n", setup_name);
		fprintf(fr, " <image>%s</image>\n", ModelData[ModelActive].image);
		fprintf(fr, " <scale>%f</scale>\n", scale);
		fprintf(fr, " <virt_view>%i</virt_view>\n", virt_view);
		fprintf(fr, " <mavlink>\n");
		for (n = 0; n < MAVLINK_PARAMETER_MAX; n++) {
			if (MavLinkVars[ModelActive][n].name[0] != 0) {
				fprintf(fr, "  <param><name>%s</name><value>%f</value></param>\n", MavLinkVars[ModelActive][n].name, MavLinkVars[ModelActive][n].value);
			}
		}
		fprintf(fr, " </mavlink>\n");
		for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
			if (RcPlugin[plugin].name[0] != 0) {
				fprintf(fr, " <plugin>\n");
				fprintf(fr, "  <name>%s</name>\n", RcPlugin[plugin].name);
				fprintf(fr, "  <title>%s</title>\n", RcPlugin[plugin].title);
				fprintf(fr, "  <text>%s</text>\n", RcPlugin[plugin].text);
				fprintf(fr, "  <type>%i</type>\n", RcPlugin[plugin].type);
				fprintf(fr, "  <option>%i</option>\n", RcPlugin[plugin].option);
				fprintf(fr, "  <open>%i</open>\n", RcPlugin[plugin].open);
				fprintf(fr, "  <x>%f</x>\n", RcPlugin[plugin].x);
				fprintf(fr, "  <y>%f</y>\n", RcPlugin[plugin].y);
				fprintf(fr, "  <x_virt>%f</x_virt>\n", RcPlugin[plugin].x_virt);
				fprintf(fr, "  <y_virt>%f</y_virt>\n", RcPlugin[plugin].y_virt);
				for (input = 0; input < MAX_INPUTS; input++) {
					fprintf(fr, "  <input>\n");
					fprintf(fr, "    <name>%s</name>\n", RcPlugin[plugin].input[input].name);
					fprintf(fr, "    <value>%i</value>\n", RcPlugin[plugin].input[input].value);
					fprintf(fr, "    <type>%i</type>\n", RcPlugin[plugin].input[input].type);
					fprintf(fr, "    <tmp_value>%f</tmp_value>\n", RcPlugin[plugin].input[input].tmp_value);
					fprintf(fr, "    <tmp_text>%s</tmp_text>\n", RcPlugin[plugin].input[input].tmp_text);
					fprintf(fr, "    <option>%i</option>\n", RcPlugin[plugin].input[input].option);
					fprintf(fr, "  </input>\n");
				}
				for (output = 0; output < MAX_OUTPUTS; output++) {
					fprintf(fr, "  <output>\n");
					fprintf(fr, "    <name>%s</name>\n", RcPlugin[plugin].output[output].name);
					fprintf(fr, "    <value>%i</value>\n", RcPlugin[plugin].output[output].value);
					fprintf(fr, "    <type>%i</type>\n", RcPlugin[plugin].output[output].type);
					fprintf(fr, "    <tmp_value>%f</tmp_value>\n", RcPlugin[plugin].output[output].tmp_value);
					fprintf(fr, "    <tmp_text>%s</tmp_text>\n", RcPlugin[plugin].output[output].tmp_text);
					fprintf(fr, "    <option>%i</option>\n", RcPlugin[plugin].output[output].option);
					fprintf(fr, "  </output>\n");
				}
				fprintf(fr, " </plugin>\n");
			}
		}
		for (link = 0; link < MAX_LINKS; link++) {
			if (RcLink[link].from[0] != 0 && RcLink[link].to[0] != 0) {
				fprintf(fr, " <link>\n");
				fprintf(fr, "  <from>%s</from>\n", RcLink[link].from);
				fprintf(fr, "  <to>%s</to>\n", RcLink[link].to);
				fprintf(fr, " </link>\n");
			}
		}
		fprintf(fr, "</rcflow>\n");
	        fclose(fr);
	}
	unsave = 0;
	return 0;
}

uint8_t rcflow_save (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	keyboard_set_callback(rcflow_save_xml);
	keyboard_set_text(setup_name);
	keyboard_set_mode(setup.view_mode);
	return 0;
}

uint8_t rcflow_plugin_del (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	del_plugin = 1 - del_plugin;
	return 0;
}

uint8_t rcflow_plugin_add (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	int n = 0;
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	select_plugin = 0;
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPlugin[plugin].name[0] == 0) {
			break;
		}
	}
	RcPlugin[plugin].type = 0;
	RcPlugin[plugin].x = -canvas_x;
	RcPlugin[plugin].y = -canvas_y;
	RcPlugin[plugin].x_virt = 0.0;
	RcPlugin[plugin].y_virt = 0.0;
	for (input = 0; input < MAX_INPUTS; input++) {
		RcPlugin[plugin].input[input].name[0] = 0;
		RcPlugin[plugin].input[input].value = 0;
		RcPlugin[plugin].input[input].type = RCFLOW_INOUT_TYPE_BILINEAR;
	}
	for (output = 0; output < MAX_INPUTS; output++) {
		RcPlugin[plugin].output[output].name[0] = 0;
		RcPlugin[plugin].output[output].value = 0;
		RcPlugin[plugin].output[output].type = RCFLOW_INOUT_TYPE_BILINEAR;
	}
	if ((uint8_t)data == RCFLOW_PLUGIN_INPUT) {
		sprintf(RcPlugin[plugin].name, "in%i", plugin);
		RcPlugin[plugin].type = RCFLOW_PLUGIN_INPUT;
		strcpy(RcPlugin[plugin].input[0].name, "val");
		strcpy(RcPlugin[plugin].input[1].name, "trm");
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_OUTPUT) {
		sprintf(RcPlugin[plugin].name, "out%i", plugin);
		RcPlugin[plugin].type = RCFLOW_PLUGIN_OUTPUT;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "trm");
		strcpy(RcPlugin[plugin].output[0].name, "val");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_ATV) {
		sprintf(RcPlugin[plugin].name, "atv%i", plugin);
		strcpy(RcPlugin[plugin].title, "ATV");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_ATV;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "up");
		RcPlugin[plugin].input[1].value = 500;
		strcpy(RcPlugin[plugin].input[2].name, "down");
		RcPlugin[plugin].input[2].value = -500;
		strcpy(RcPlugin[plugin].input[3].name, "sw");
		RcPlugin[plugin].input[3].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[3].value = 1000;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_EXPO) {
		sprintf(RcPlugin[plugin].name, "expo%i", plugin);
		strcpy(RcPlugin[plugin].title, "EXPO");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_EXPO;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "up");
		RcPlugin[plugin].input[1].value = 500;
		strcpy(RcPlugin[plugin].input[2].name, "down");
		RcPlugin[plugin].input[2].value = -500;
		strcpy(RcPlugin[plugin].input[3].name, "sw");
		RcPlugin[plugin].input[3].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[3].value = 1000;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_BUTTERFLY) {
		sprintf(RcPlugin[plugin].name, "bfly%i", plugin);
		strcpy(RcPlugin[plugin].title, "BUTTERFLY");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_BUTTERFLY;
		strcpy(RcPlugin[plugin].input[0].name, "aileron");
		strcpy(RcPlugin[plugin].input[1].name, "flaps");
		strcpy(RcPlugin[plugin].input[2].name, "elev");
		strcpy(RcPlugin[plugin].input[3].name, "ruder");
		strcpy(RcPlugin[plugin].input[4].name, "break");
		strcpy(RcPlugin[plugin].input[5].name, "sw");
		RcPlugin[plugin].input[5].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[5].value = 1000;
		strcpy(RcPlugin[plugin].output[0].name, "aile_l");
		strcpy(RcPlugin[plugin].output[1].name, "flap_l");
		strcpy(RcPlugin[plugin].output[2].name, "flap_r");
		strcpy(RcPlugin[plugin].output[3].name, "aile_r");
		strcpy(RcPlugin[plugin].output[4].name, "elev_out");
		strcpy(RcPlugin[plugin].output[5].name, "ruder_out");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_MULTIPLEX_IN) {
		sprintf(RcPlugin[plugin].name, "mpxi%i", plugin);
		strcpy(RcPlugin[plugin].title, "MPX-In");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_MULTIPLEX_IN;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "sel");
		strcpy(RcPlugin[plugin].output[0].name, "out1");
		strcpy(RcPlugin[plugin].output[1].name, "out2");
		strcpy(RcPlugin[plugin].output[2].name, "out3");
		strcpy(RcPlugin[plugin].output[3].name, "out4");
		strcpy(RcPlugin[plugin].output[4].name, "out5");
		strcpy(RcPlugin[plugin].output[5].name, "out6");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_MULTIPLEX_OUT) {
		sprintf(RcPlugin[plugin].name, "mpxo%i", plugin);
		strcpy(RcPlugin[plugin].title, "MPX-Out");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_MULTIPLEX_OUT;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		strcpy(RcPlugin[plugin].input[1].name, "in2");
		strcpy(RcPlugin[plugin].input[2].name, "in3");
		strcpy(RcPlugin[plugin].input[3].name, "in4");
		strcpy(RcPlugin[plugin].input[4].name, "in5");
		strcpy(RcPlugin[plugin].input[5].name, "in6");
		strcpy(RcPlugin[plugin].input[6].name, "sel");
		strcpy(RcPlugin[plugin].output[0].name, "out");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_MULTIVALUE) {
		sprintf(RcPlugin[plugin].name, "mulv%i", plugin);
		strcpy(RcPlugin[plugin].title, "Multival");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_MULTIVALUE;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		strcpy(RcPlugin[plugin].input[1].name, "sel");
		strcpy(RcPlugin[plugin].output[0].name, "out");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_MIXER) {
		sprintf(RcPlugin[plugin].name, "mix%i", plugin);
		strcpy(RcPlugin[plugin].title, "Mixer");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_MIXER;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		strcpy(RcPlugin[plugin].input[1].name, "val1");
		RcPlugin[plugin].input[1].value = 1000;
		strcpy(RcPlugin[plugin].input[2].name, "in2");
		strcpy(RcPlugin[plugin].input[3].name, "val2");
		RcPlugin[plugin].input[3].value = 1000;
		strcpy(RcPlugin[plugin].input[4].name, "sw");
		RcPlugin[plugin].input[4].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[4].value = 1000;
		strcpy(RcPlugin[plugin].input[5].name, "zero");
		RcPlugin[plugin].input[5].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[5].value = 1000;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_LIMIT) {
		sprintf(RcPlugin[plugin].name, "lim%i", plugin);
		strcpy(RcPlugin[plugin].title, "Limit");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_LIMIT;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "max");
		RcPlugin[plugin].input[1].value = 1000;
		strcpy(RcPlugin[plugin].input[2].name, "min");
		RcPlugin[plugin].input[2].value = -1000;
		strcpy(RcPlugin[plugin].input[3].name, "sw");
		RcPlugin[plugin].input[3].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[3].value = 1000;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_DELAY) {
		sprintf(RcPlugin[plugin].name, "slw%i", plugin);
		strcpy(RcPlugin[plugin].title, "Slow");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_DELAY;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "up");
		RcPlugin[plugin].input[1].value = -950;
		RcPlugin[plugin].input[1].type = RCFLOW_INOUT_TYPE_LINEAR;
		strcpy(RcPlugin[plugin].input[2].name, "down");
		RcPlugin[plugin].input[2].value = -950;
		RcPlugin[plugin].input[2].type = RCFLOW_INOUT_TYPE_LINEAR;
		strcpy(RcPlugin[plugin].input[3].name, "sw");
		RcPlugin[plugin].input[3].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[3].value = 1000;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_CURVE) {
		sprintf(RcPlugin[plugin].name, "crv%i", plugin);
		strcpy(RcPlugin[plugin].title, "Curve");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_CURVE;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		strcpy(RcPlugin[plugin].input[1].name, "in2");
		for (n = 0; n < MAX_CURVE_POINTS; n++) {
			sprintf(RcPlugin[plugin].input[n + 2].name, "v%i", n + 1);
			RcPlugin[plugin].input[n + 2].value = n * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
			RcPlugin[plugin].input[n + 2].type = RCFLOW_INOUT_TYPE_CURVE;
		}
		strcpy(RcPlugin[plugin].output[0].name, "out1");
		strcpy(RcPlugin[plugin].output[1].name, "out2");
		strcpy(RcPlugin[plugin].output[2].name, "rev1");
		strcpy(RcPlugin[plugin].output[3].name, "rev2");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_H120) {
		sprintf(RcPlugin[plugin].name, "h120_%i", plugin);
		strcpy(RcPlugin[plugin].title, "Heli-120");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_H120;
		strcpy(RcPlugin[plugin].input[0].name, "roll");
		strcpy(RcPlugin[plugin].input[1].name, "nick");
		strcpy(RcPlugin[plugin].input[2].name, "pitch");
		strcpy(RcPlugin[plugin].output[0].name, "roll_l");
		strcpy(RcPlugin[plugin].output[1].name, "roll_r");
		strcpy(RcPlugin[plugin].output[2].name, "nick");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_SWITCH) {
		sprintf(RcPlugin[plugin].name, "switch_%i", plugin);
		sprintf(RcPlugin[plugin].name, "Switch_%i", plugin);
		RcPlugin[plugin].type = RCFLOW_PLUGIN_SWITCH;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		strcpy(RcPlugin[plugin].input[1].name, "in2");
		strcpy(RcPlugin[plugin].input[2].name, "zero");
		RcPlugin[plugin].input[2].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[2].value = 1000;
		strcpy(RcPlugin[plugin].input[3].name, "sw");
		RcPlugin[plugin].input[3].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].input[3].value = -1000;
		strcpy(RcPlugin[plugin].output[0].name, "out1a");
		strcpy(RcPlugin[plugin].output[1].name, "out2a");
		strcpy(RcPlugin[plugin].output[2].name, "out1b");
		strcpy(RcPlugin[plugin].output[3].name, "out2b");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_RANGESW) {
		sprintf(RcPlugin[plugin].name, "rangesw_%i", plugin);
		sprintf(RcPlugin[plugin].name, "RangeSW");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_RANGESW;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "min");
		RcPlugin[plugin].input[1].type = RCFLOW_INOUT_TYPE_LINEAR;
		RcPlugin[plugin].input[1].value = -333;
		strcpy(RcPlugin[plugin].input[2].name, "max");
		RcPlugin[plugin].input[2].type = RCFLOW_INOUT_TYPE_LINEAR;
		RcPlugin[plugin].input[2].value = 333;
		strcpy(RcPlugin[plugin].output[0].name, "out1");
		strcpy(RcPlugin[plugin].output[1].name, "rev1");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_TIMER) {
		sprintf(RcPlugin[plugin].name, "timer_%i", plugin);
		sprintf(RcPlugin[plugin].name, "Timer");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_TIMER;
		strcpy(RcPlugin[plugin].input[0].name, "set");
		RcPlugin[plugin].input[0].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[0].value = -1000;
		strcpy(RcPlugin[plugin].input[1].name, "reset");
		RcPlugin[plugin].input[1].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[1].value = -1000;
		strcpy(RcPlugin[plugin].input[2].name, "max");
		RcPlugin[plugin].input[2].type = RCFLOW_INOUT_TYPE_LINEAR;
		RcPlugin[plugin].input[2].value = 500;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
		strcpy(RcPlugin[plugin].output[2].name, "time");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_AND) {
		sprintf(RcPlugin[plugin].name, "and_%i", plugin);
		sprintf(RcPlugin[plugin].name, "LOGIC_AND");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_AND;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		RcPlugin[plugin].input[0].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[0].value = -1000;
		strcpy(RcPlugin[plugin].input[1].name, "in2");
		RcPlugin[plugin].input[1].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[1].value = -1000;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_OR) {
		sprintf(RcPlugin[plugin].name, "or_%i", plugin);
		sprintf(RcPlugin[plugin].name, "LOGIC_OR");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_OR;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		RcPlugin[plugin].input[0].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[0].value = -1000;
		strcpy(RcPlugin[plugin].input[1].name, "in2");
		RcPlugin[plugin].input[1].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].input[1].value = -1000;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_PULSE) {
		sprintf(RcPlugin[plugin].name, "or_%i", plugin);
		sprintf(RcPlugin[plugin].name, "Pulse");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_PULSE;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "time");
		RcPlugin[plugin].input[1].type = RCFLOW_INOUT_TYPE_LINEAR;
		RcPlugin[plugin].input[1].value = 100;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
		strcpy(RcPlugin[plugin].output[2].name, "timer");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_SINUS) {
		sprintf(RcPlugin[plugin].name, "sinus_%i", plugin);
		sprintf(RcPlugin[plugin].name, "Sinus");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_SINUS;
		strcpy(RcPlugin[plugin].input[0].name, "speed");
		RcPlugin[plugin].input[0].type = RCFLOW_INOUT_TYPE_LINEAR;
		RcPlugin[plugin].input[0].value = 100;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		strcpy(RcPlugin[plugin].output[1].name, "rev");
		strcpy(RcPlugin[plugin].output[2].name, "dir");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_ALARM) {
		sprintf(RcPlugin[plugin].name, "alarm_%i", plugin);
		sprintf(RcPlugin[plugin].name, "Alarm");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_ALARM;
		strcpy(RcPlugin[plugin].input[0].name, "in");
		strcpy(RcPlugin[plugin].input[1].name, "min");
		RcPlugin[plugin].input[1].type = RCFLOW_INOUT_TYPE_LINEAR;
		RcPlugin[plugin].input[1].value = -333;
		strcpy(RcPlugin[plugin].input[2].name, "max");
		RcPlugin[plugin].input[2].type = RCFLOW_INOUT_TYPE_LINEAR;
		RcPlugin[plugin].input[2].value = 333;
		strcpy(RcPlugin[plugin].output[0].name, "out1");
		strcpy(RcPlugin[plugin].output[1].name, "rev1");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_VADC) {
		sprintf(RcPlugin[plugin].name, "vadc%i", plugin);
		RcPlugin[plugin].type = RCFLOW_PLUGIN_VADC;
		strcpy(RcPlugin[plugin].output[0].name, "out");
	} else if ((uint8_t)data == RCFLOW_PLUGIN_VSW) {
		sprintf(RcPlugin[plugin].name, "vsw%i", plugin);
		RcPlugin[plugin].type = RCFLOW_PLUGIN_VSW;
		strcpy(RcPlugin[plugin].output[0].name, "out");
		RcPlugin[plugin].output[0].type = RCFLOW_INOUT_TYPE_ONOFF;
		RcPlugin[plugin].output[0].value = -1000;
	} else if ((uint8_t)data == RCFLOW_PLUGIN_TCL) {
		sprintf(RcPlugin[plugin].name, "tcl%i", plugin);
		RcPlugin[plugin].type = RCFLOW_PLUGIN_TCL;
		strcpy(RcPlugin[plugin].input[0].name, "in1");
		strcpy(RcPlugin[plugin].input[1].name, "in2");
		strcpy(RcPlugin[plugin].input[2].name, "in3");
		strcpy(RcPlugin[plugin].input[3].name, "in4");
		strcpy(RcPlugin[plugin].input[4].name, "in5");
		strcpy(RcPlugin[plugin].input[5].name, "in6");
		strcpy(RcPlugin[plugin].input[6].name, "in7");
		strcpy(RcPlugin[plugin].input[7].name, "in8");
		strcpy(RcPlugin[plugin].output[0].name, "out1");
		strcpy(RcPlugin[plugin].output[1].name, "out2");
		strcpy(RcPlugin[plugin].output[2].name, "out3");
		strcpy(RcPlugin[plugin].output[3].name, "out4");
		strcpy(RcPlugin[plugin].output[4].name, "out5");
		strcpy(RcPlugin[plugin].output[5].name, "out6");
		strcpy(RcPlugin[plugin].output[6].name, "out7");
		strcpy(RcPlugin[plugin].output[7].name, "out8");
		RcPlugin[plugin].text[0] = 0;
		strcpy(RcPlugin[plugin].text, "set Output(0) $Input(0); set Output(1) [expr $Input(0) * -1]");
	}
	lock = 0;
	unsave = 1;
	rcflow_update_plugin(plugin);
	return 0;
}

uint8_t rcflow_plugin_select (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	reset_buttons();
	select_plugin = 1;
	return 0;
}


uint8_t rcflow_plugin_move (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	static float move_diff_x = 0.0;
	static float move_diff_y = 0.0;
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	uint8_t link = 0;
	char tmp_str[64];
	char tmp_str2[64];
	if (del_plugin == 1) {
		for (link = 0; link < MAX_LINKS; link++) {
			if (RcLink[link].from[0] != 0 && RcLink[link].to[0] != 0) {
				sprintf(tmp_str, "%s;", RcPlugin[(int)data].name);
				sprintf(tmp_str2, "%s;", RcLink[link].from);
				if (strncmp(tmp_str, tmp_str2, strlen(tmp_str)) == 0) {
					RcLink[link].from[0] = 0;
					RcLink[link].to[0] = 0;
				}
				sprintf(tmp_str2, "%s;", RcLink[link].to);
				if (strncmp(tmp_str, tmp_str2, strlen(tmp_str)) == 0) {
					RcLink[link].from[0] = 0;
					RcLink[link].to[0] = 0;
				}
			}
		}
		RcPlugin[(int)data].name[0] = 0;
		RcPlugin[(int)data].title[0] = 0;
		del_plugin = 0;
		unsave = 1;
		rcflow_update_plugin((int)data);
		return 0;
	}
	if (lock == 1) {
		lock_timer = 50;
		return 0;
	}
	x -= canvas_x;
	y -= canvas_y;
	// 50px raster
	x = (float)((int)(x * 50)) / 50.0;
	y = (float)((int)(y * 50)) / 50.0;
	if (action == 0) {
		if (virt_view == 1) {
			move_diff_x = RcPlugin[(int)data].x_virt * scale - x;
			move_diff_y = RcPlugin[(int)data].y_virt * scale - y;
		} else {
			move_diff_x = RcPlugin[(int)data].x * scale - x;
			move_diff_y = RcPlugin[(int)data].y * scale - y;
		}
	}
	x += move_diff_x;
	y += move_diff_y;
	if (virt_view == 1) {
		RcPlugin[(int)data].x_virt = x / scale;
		RcPlugin[(int)data].y_virt = y / scale;
	} else {
		RcPlugin[(int)data].x = x / scale;
		RcPlugin[(int)data].y = y / scale;
	}
//	SDL_Log("rcflow: MOVE: %0.2f, %0.2f\n", x, y);

	rcflow_plugin_move_id = (int)data;
	rcflow_plugin_move_timer = 50;

	unsave = 1;
	return 0;
}

int16_t rcflow_value_limit (int16_t value, int16_t min, int16_t max) {
	if (value >= max) {
		return max;
	}
	if (value <= min) {
		return min;
	}
	return value;
}

float rcflow_value_limit_float (float value, float min, float max) {
	if (value >= max) {
		return max;
	}
	if (value <= min) {
		return min;
	}
	return value;
}

uint8_t rcflow_canvas_center (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	canvas_x = 0.0;
	canvas_y = 0.0;
	return 0;
}

uint8_t rcflow_canvas_move (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	} else if (button == 3) {
		canvas_x = 0.0;
		canvas_y = 0.0;
		return 0;
	}
	canvas_x += x / scale;
	canvas_y += y / scale;
	canvas_x = rcflow_value_limit_float(canvas_x, -1.5, 1.5);
	canvas_y = rcflow_value_limit_float(canvas_y, -1.5, 1.5);
	return 0;
}

uint8_t rcflow_link_add (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	uint8_t link = 0;
	for (link = 0; link < MAX_LINKS; link++) {
		if (RcLink[link].from[0] != 0 && RcLink[link].to[0] != 0) {
			if (strcmp(RcLink[link].from, link_select_from) == 0 && strcmp(RcLink[link].to, link_select_to) == 0) {
				RcLink[link].from[0] = 0;
				RcLink[link].to[0] = 0;
				link_select_from[0] = 0;
				link_select_to[0] = 0;
				unsave = 1;
				rcflow_update_link(link);
				return 0;
			}
		}
	}
	for (link = 0; link < MAX_LINKS; link++) {
		if (RcLink[link].from[0] == 0 || RcLink[link].to[0] == 0) {
			strcpy(RcLink[link].from, link_select_from);
			strcpy(RcLink[link].to, link_select_to);
			link_select_from[0] = 0;
			link_select_to[0] = 0;
			unsave = 1;
			rcflow_update_link(link);
			return 0;
		}
	}
	return 0;
}

uint8_t rcflow_link_mark (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	uint8_t plugin = 0;
	uint8_t input = 0;
	char tmp_str[64];
	if (button == 4 || button == 5) {
		for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
			if (RcPlugin[plugin].name[0] != 0) {
				for (input = 0; input < MAX_INPUTS; input++) {
					if (RcPlugin[plugin].input[input].name[0] != 0) {
						sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].input[input].name);
						if (strcmp(name, tmp_str) == 0) {
							if (button == 4) {
								if (RcPlugin[plugin].input[input].type == RCFLOW_INOUT_TYPE_ONOFF) {
									if (RcPlugin[plugin].input[input].value < 0) {
										RcPlugin[plugin].input[input].value = 1000;
									}
								} else {
									RcPlugin[plugin].input[input].value += 1;
								}
							} else if (button == 5) {
								if (RcPlugin[plugin].input[input].type == RCFLOW_INOUT_TYPE_ONOFF) {
									if (RcPlugin[plugin].input[input].value > 0) {
										RcPlugin[plugin].input[input].value = -1000;
									}
								} else {
									RcPlugin[plugin].input[input].value -= 1;
								}
							}
							break;
						}
					}
				}
			}
		}
	} else if (button == 3) {
		rcflow_link_add("", 0.0, 0.0, 0, 0.0, 0);
	} else if (button == 1) {
		if ((int)data == 0) {
			if (strcmp(link_select_from, name) == 0) {
				link_select_from[0] = 0;
			} else {
				strcpy(link_select_from, name);
			}
		} else {
			if (strcmp(link_select_to, name) == 0) {
				link_select_to[0] = 0;
			} else {
				strcpy(link_select_to, name);
			}
		}
	}
	if (link_select_from[0] != 0 && link_select_to[0] != 0) {
		rcflow_link_add("", 0.0, 0.0, 0, 0.0, 0);
	}
	return 0;
}

uint8_t rcflow_plugin_title_set (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	strcpy(RcPlugin[edit_title].title, name);
	edit_title = -1;
	return 0;
}

uint8_t rcflow_plugin_title_edit (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	if (button != 3) {
		return 1;
	}
	edit_title = (int16_t)data;
	keyboard_set_callback(rcflow_plugin_title_set);
	if (RcPlugin[edit_title].title[0] != 0) {
		keyboard_set_text(RcPlugin[edit_title].title);
	} else {
		keyboard_set_text(RcPlugin[edit_title].name);
	}
	keyboard_set_mode(setup.view_mode);
	return 0;
}

uint8_t rcflow_plugin_max_get (uint8_t id) {
	uint8_t input_max = 1;
	uint8_t output_max = 1;
	uint8_t num = 0;
	uint8_t num_max = 0;
	for (num = 0; num < MAX_INPUTS; num++) {
		if (RcPlugin[id].input[num].name[0] != 0 && RcPlugin[id].input[num].type != RCFLOW_INOUT_TYPE_TEMP) {
			input_max++;
		}
	}
	for (num = 0; num < MAX_OUTPUTS; num++) {
		if (RcPlugin[id].output[num].name[0] != 0 && RcPlugin[id].output[num].type != RCFLOW_INOUT_TYPE_TEMP) {
			output_max++;
		}
	}
	num_max = input_max;
	if (num_max < output_max) {
		num_max = output_max;
	}
	return num_max;
}

uint8_t rcflow_plugin_curve_preset (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	uint8_t plugin = 0;
	uint8_t n = 0;
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPlugin[plugin].name[0] != 0) {
			if (strcmp(RcPlugin[plugin].name, name + 7) == 0) {
				if (RcPlugin[plugin].input[2].value == -1000 && RcPlugin[plugin].input[MAX_CURVE_POINTS / 2 + 2].value == 0 && RcPlugin[plugin].input[MAX_CURVE_POINTS + 1].value == 1000) {
					for (n = 0; n <= MAX_CURVE_POINTS / 2; n++) {
						RcPlugin[plugin].input[n + 2].value = n * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
						RcPlugin[plugin].input[MAX_CURVE_POINTS - n +1].value = n * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
					}
				} else if (RcPlugin[plugin].input[2].value == -1000 && RcPlugin[plugin].input[MAX_CURVE_POINTS / 2 + 2].value == 0 && RcPlugin[plugin].input[MAX_CURVE_POINTS + 1].value == -1000) {
					for (n = 0; n <= MAX_CURVE_POINTS / 2; n++) {
						RcPlugin[plugin].input[n + 2].value = n * 4000 / (MAX_CURVE_POINTS - 1) - 1000;
						RcPlugin[plugin].input[MAX_CURVE_POINTS - n + 1].value = n * 4000 / (MAX_CURVE_POINTS - 1) - 1000;
					}
				} else {
					for (n = 0; n < MAX_CURVE_POINTS; n++) {
						RcPlugin[plugin].input[n + 2].value = n * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
					}
				}
			}
		}
	}
	return 0;
}

uint8_t rcflow_switch_change (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	char tmp_str[64];
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPlugin[plugin].name[0] != 0) {
			for (input = 0; input < MAX_INPUTS; input++) {
				if (RcPlugin[plugin].input[input].name[0] != 0) {
					sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].input[input].name);
					if (strcmp(tmp_str, name + 3) == 0) {
						if (RcPlugin[plugin].input[input].type == RCFLOW_INOUT_TYPE_ONOFF) {
							if (RcPlugin[plugin].input[input].value < 0) {
								RcPlugin[plugin].input[input].value = 1000;
							} else {
								RcPlugin[plugin].input[input].value = -1000;
							}
						} else if (RcPlugin[plugin].input[input].type == RCFLOW_INOUT_TYPE_SWITCH) {
							if (RcPlugin[plugin].input[input].value < -333) {
								RcPlugin[plugin].input[input].value = -1000;
							} else if (RcPlugin[plugin].input[input].value > 333) {
								RcPlugin[plugin].input[input].value = 1000;
							} else {
								RcPlugin[plugin].input[input].value = 0;
							}
						}
						unsave = 1;
						rcflow_update_plugin(plugin);
						return 0;
					}
				}
			}
			for (output = 0; output < MAX_OUTPUTS; output++) {
				if (RcPlugin[plugin].output[output].name[0] != 0) {
					sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].output[output].name);
					if (strcmp(tmp_str, name + 3) == 0) {
						if (RcPlugin[plugin].output[output].type == RCFLOW_INOUT_TYPE_ONOFF) {
							if (RcPlugin[plugin].output[output].value < 0) {
								RcPlugin[plugin].output[output].value = 1000;
							} else if (RcPlugin[plugin].output[output].value < 500) {
								RcPlugin[plugin].output[output].value = 1000;
							} else {
								RcPlugin[plugin].output[output].value = -1000;
							}
						} else if (RcPlugin[plugin].output[output].type == RCFLOW_INOUT_TYPE_SWITCH) {
							if (RcPlugin[plugin].output[output].value < -333) {
								RcPlugin[plugin].output[output].value = -1000;
							} else if (RcPlugin[plugin].output[output].value > 333) {
								RcPlugin[plugin].output[output].value = 1000;
							} else {
								RcPlugin[plugin].output[output].value = 0;
							}
						}
						unsave = 1;
						rcflow_update_plugin(plugin);
						return 0;
					}
				}
			}
		}
	}
	return 0;
}

uint8_t rcflow_slider_move (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	char tmp_str[64];
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	float xp = 0.0;
	if (virt_view == 0) {
		x -= canvas_x * scale;
		y -= canvas_y * scale;
	}
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPlugin[plugin].name[0] != 0) {
			if (virt_view == 1) {
				xp = RcPlugin[plugin].x_virt * scale;
			} else {
				xp = RcPlugin[plugin].x * scale;
			}
			for (input = 0; input < MAX_INPUTS; input++) {
				if (RcPlugin[plugin].input[input].name[0] != 0) {
					sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].input[input].name);
					if (strcmp(tmp_str, name + 7) == 0) {
						if (RcPlugin[plugin].input[input].type == RCFLOW_INOUT_TYPE_ONOFF) {
							if ((xp - x) * -2000 / SLIDER_WIDTH / scale > 500) {
								RcPlugin[plugin].input[input].value = 1000;
							} else if ((xp - x) * -2000 / SLIDER_WIDTH / scale < -500) {
								RcPlugin[plugin].input[input].value = -1000;
							}
						} else if (RcPlugin[plugin].input[input].type == RCFLOW_INOUT_TYPE_SWITCH) {
							if ((xp - x) * -2000 / SLIDER_WIDTH / scale > 333) {
								RcPlugin[plugin].input[input].value = 1000;
							} else if ((xp - x) * -2000 / SLIDER_WIDTH / scale < -333) {
								RcPlugin[plugin].input[input].value = -1000;
							} else {
								RcPlugin[plugin].input[input].value = 0;
							}
						} else {
							if (button == 3) {
								RcPlugin[plugin].input[input].value = 0;
							} else {
								RcPlugin[plugin].input[input].value = (xp - x) * -2000 / SLIDER_WIDTH / scale;
							}
							RcPlugin[plugin].input[input].value = (float)((int)rcflow_value_limit(RcPlugin[plugin].input[input].value, -1000, 1000));
						}
						unsave = 1;
						rcflow_update_plugin(plugin);
						return 0;
					}
				}
			}
			for (output = 0; output < MAX_OUTPUTS; output++) {
				if (RcPlugin[plugin].output[output].name[0] != 0) {
					sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].output[output].name);
					if (strcmp(tmp_str, name + 7) == 0) {
						if (RcPlugin[plugin].output[output].type == RCFLOW_INOUT_TYPE_ONOFF) {
							if ((xp - x) * -2000 / SLIDER_WIDTH > 500) {
								RcPlugin[plugin].output[output].value = 1000;
							} else if ((xp - x) * -2000 / SLIDER_WIDTH < -500) {
								RcPlugin[plugin].output[output].value = -1000;
							}
						} else if (RcPlugin[plugin].output[output].type == RCFLOW_INOUT_TYPE_SWITCH) {
							if ((xp - x) * -2000 / SLIDER_WIDTH > 333) {
								RcPlugin[plugin].output[output].value = 1000;
							} else if ((xp - x) * -2000 / SLIDER_WIDTH < -333) {
								RcPlugin[plugin].output[output].value = -1000;
							} else {
								RcPlugin[plugin].output[output].value = 0;
							}
						} else {
							if (button == 3) {
								RcPlugin[plugin].output[output].value = 0;
							} else {
								RcPlugin[plugin].output[output].value = (xp - x) * -2000 / SLIDER_WIDTH;
							}
							RcPlugin[plugin].output[output].value = (float)((int)rcflow_value_limit(RcPlugin[plugin].output[output].value, -1000, 1000));
						}
						unsave = 1;
						rcflow_update_plugin(plugin);
						return 0;
					}
				}
			}

		}
	}
	return 0;
}

void rcflow_draw_switch (ESContext *esContext, char *name, float x, float y, float w, float value, uint8_t type) {
	char tmp_str[1024];
	if (value > 0) {
#ifndef SIMPLE_DRAW
//		draw_box_f3c2(esContext, (x - (w / 2)) * scale, (y) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.025) * scale, 0.001, 255, 255, 255, 200, 25, 25, 25, 150);
//		draw_box_f3c2(esContext, (x - (w / 2)) * scale, (y + 0.025) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.05) * scale, 0.001, 25, 25, 25, 150, 255, 255, 255, 200);

		sprintf(tmp_str, "%s/textures/led-on.png", BASE_DIR);
		draw_image_f3(esContext, (x - 0.04 - CONN_RADIUS) * scale, (y + 0.025 - CONN_RADIUS) * scale, (x - 0.04 + CONN_RADIUS) * scale, (y + 0.025 + CONN_RADIUS) * scale, 0.02, tmp_str);

		sprintf(tmp_str, "%s/textures/sw-on.png", BASE_DIR);
		draw_image_f3(esContext, (x + 0.04 - CONN_RADIUS) * scale, (y + 0.025 - CONN_RADIUS) * scale, (x + 0.04 + CONN_RADIUS) * scale, (y + 0.025 + CONN_RADIUS) * scale, 0.02, tmp_str);

		sprintf(tmp_str, "sw_%s", name);
		draw_text_button(esContext, tmp_str, setup.view_mode, "        ", FONT_GREEN, x * scale, y * scale, 0.002, 0.05 * scale, ALIGN_CENTER, ALIGN_TOP, rcflow_switch_change, 0.0);
#else
		sprintf(tmp_str, "sw_%s", name);
		draw_text_button(esContext, tmp_str, setup.view_mode, "ON", FONT_GREEN, x * scale, y * scale, 0.002, 0.05 * scale, ALIGN_CENTER, ALIGN_TOP, rcflow_switch_change, 0.0);
#endif
	} else {
#ifndef SIMPLE_DRAW
//		draw_box_f3c2(esContext, (x - (w / 2)) * scale, (y) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.025) * scale, 0.001, 25, 25, 25, 150, 255, 255, 255, 200);
//		draw_box_f3c2(esContext, (x - (w / 2)) * scale, (y + 0.025) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.05) * scale, 0.001, 255, 255, 255, 200, 25, 25, 25, 150);

		sprintf(tmp_str, "%s/textures/led-off.png", BASE_DIR);
		draw_image_f3(esContext, (x - 0.04 - CONN_RADIUS) * scale, (y + 0.025 - CONN_RADIUS) * scale, (x - 0.04 + CONN_RADIUS) * scale, (y + 0.025 + CONN_RADIUS) * scale, 0.02, tmp_str);

		sprintf(tmp_str, "%s/textures/sw-off.png", BASE_DIR);
		draw_image_f3(esContext, (x + 0.04 - CONN_RADIUS) * scale, (y + 0.025 - CONN_RADIUS) * scale, (x + 0.04 + CONN_RADIUS) * scale, (y + 0.025 + CONN_RADIUS) * scale, 0.02, tmp_str);

		sprintf(tmp_str, "sw_%s", name);
		draw_text_button(esContext, tmp_str, setup.view_mode, "        ", FONT_GREEN, x * scale, y * scale, 0.002, 0.05 * scale, ALIGN_CENTER, ALIGN_TOP, rcflow_switch_change, 0.0);
#else
		sprintf(tmp_str, "sw_%s", name);
		draw_text_button(esContext, tmp_str, setup.view_mode, "OFF", FONT_WHITE, x * scale, y * scale, 0.002, 0.05 * scale, ALIGN_CENTER, ALIGN_TOP, rcflow_switch_change, 0.0);
#endif
	}
}

void rcflow_draw_slider (ESContext *esContext, char *name, float x, float y, float w, float value, float value2, uint8_t type) {
	uint8_t n = 0;
	char tmp_str[1024];
	if (type != 3) {
#ifdef SIMPLE_DRAW
		draw_box_f3(esContext, (x - (w / 2)) * scale, (y) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.05) * scale, 0.001, 255, 255, 255, 255);
#endif
		if (type == RCFLOW_INOUT_TYPE_SWITCH) {
			if (value < -333) {
				value = -1000;
				value2 = -333;
				draw_box_f3(esContext, (x + (value * w / 2000)) * scale, y * scale, 0.001, (x + (value2 * w / 2000)) * scale, (y + 0.05) * scale, 0.001, 255, 255, 155, 127);
			} else if (value > 333) {
				value = 333;
				value2 = 1000;
				draw_box_f3(esContext, (x + (value * w / 2000)) * scale, y * scale, 0.001, (x + (value2 * w / 2000)) * scale, (y + 0.05) * scale, 0.001, 5, 125, 25, 127);
			} else {
				value = -333;
				value2 = 333;
				draw_box_f3(esContext, (x + (value * w / 2000)) * scale, y * scale, 0.001, (x + (value2 * w / 2000)) * scale, (y + 0.05) * scale, 0.001, 145, 5, 25, 127);
			}
#ifndef SIMPLE_DRAW
			draw_box_f3(esContext, (x - (w / 2)) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.05 - 0.009) * scale, 0.001, 50, 50, 50, 255);
			draw_line_f3(esContext, (x - (w / 2)) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.009) * scale, 0.001, 0, 0, 0, 255);
			draw_line_f3(esContext, (x - (w / 2)) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2)) * scale, (y + 0.05 - 0.009) * scale, 0.001, 0, 0, 0, 255);
			draw_line_f3(esContext, (x - (w / 2)) * scale, (y + 0.05 - 0.009) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.05 - 0.009) * scale, 0.001, 150, 150, 150, 255);
			draw_line_f3(esContext, (x - (w / 2) + w) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2) + w) * scale, (y + 0.05 - 0.009) * scale, 0.001, 150, 150, 150, 255);
			draw_box_f3(esContext, (x - (w / 2) + 0.035 - 0.001) * scale, (y + 0.025 - 0.003) * scale, 0.001, (x - (w / 2) + w - 0.035 + 0.01) * scale, (y + 0.025 + 0.003) * scale, 0.001, 0, 0, 0, 255);
			for (n = 0; n <= 6; n++) {
				#define SHADOW_OFFSET 0.002
				draw_box_f3(esContext, (x - (w / 2.0) + (w * (float)n / 6.0) - 0.001 + SHADOW_OFFSET) * scale, (y + 0.001 + SHADOW_OFFSET) * scale, 0.001, (x - (w / 2.0) + (w * (float)n / 6.0) + 0.001 + SHADOW_OFFSET) * scale, (y + 0.007 + SHADOW_OFFSET) * scale, 0.001, 2, 2, 2, 255);
				draw_box_f3(esContext, (x - (w / 2.0) + (w * (float)n / 6.0) - 0.001) * scale, (y + 0.001) * scale, 0.001, (x - (w / 2.0) + (w * (float)n / 6.0) + 0.001) * scale, (y + 0.007) * scale, 0.001, 255, 255, 255, 255);
			}
			sprintf(tmp_str, "%s/textures/slider.png", BASE_DIR);
			draw_image_f3(esContext, (x + (value * w / 2000)) * scale, (y + 0.01) * scale, (x + (value2 * w / 2000)) * scale, (y + 0.05 - 0.01) * scale, 0.02, tmp_str);
#endif
			draw_line_f3(esContext, (x - (w / 2) + w / 3.0 * 1.0) * scale, (y - 0.001) * scale, 0.001, (x - (w / 2) + w / 3.0 * 1.0) * scale, (y + 0.05 + 0.001) * scale, 0.001, 127, 127, 127, 255);
			draw_line_f3(esContext, (x - (w / 2) + w / 3.0 * 2.0) * scale, (y - 0.001) * scale, 0.001, (x - (w / 2) + w / 3.0 * 2.0) * scale, (y + 0.05 + 0.001) * scale, 0.001, 127, 127, 127, 255);
		} else {
			if (type == RCFLOW_INOUT_TYPE_LINEAR) {
				draw_box_f3(esContext, (x - (w / 2)) * scale, y * scale, 0.001, (x + (value * w / 2000)) * scale, (y + 0.05) * scale, 0.001, 5, 145, 185, 255);
			} else {
				draw_box_f3(esContext, (x - (w / 2) + w / 2.0) * scale, y * scale, 0.001, (x - (w / 2) + w / 2.0 + (value * w / 2000)) * scale, (y + 0.05) * scale, 0.001, 5, 145, 185, 127);
				draw_box_f3(esContext, (x - (w / 2) + w / 2.0) * scale, y * scale, 0.001, (x - (w / 2) + w / 2.0 + (value2 * w / 2000)) * scale, (y + 0.05) * scale, 0.001, 175, 145, 85, 127);
			}
#ifndef SIMPLE_DRAW
			draw_box_f3(esContext, (x - (w / 2) - 0.032) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2) + w + 0.032) * scale, (y + 0.05 - 0.009) * scale, 0.001, 50, 50, 50, 255);
			draw_line_f3(esContext, (x - (w / 2) - 0.032) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2) + w + 0.032) * scale, (y + 0.009) * scale, 0.001, 0, 0, 0, 255);
			draw_line_f3(esContext, (x - (w / 2) - 0.032) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2) - 0.032) * scale, (y + 0.05 - 0.009) * scale, 0.001, 0, 0, 0, 255);
			draw_line_f3(esContext, (x - (w / 2) - 0.032) * scale, (y + 0.05 - 0.009) * scale, 0.001, (x - (w / 2) + w + 0.032) * scale, (y + 0.05 - 0.009) * scale, 0.001, 150, 150, 150, 255);
			draw_line_f3(esContext, (x - (w / 2) + w + 0.032) * scale, (y + 0.009) * scale, 0.001, (x - (w / 2) + w + 0.032) * scale, (y + 0.05 - 0.009) * scale, 0.001, 150, 150, 150, 255);
			draw_box_f3(esContext, (x - (w / 2) - 0.01) * scale, (y + 0.025 - 0.003) * scale, 0.001, (x - (w / 2) + w + 0.01) * scale, (y + 0.025 + 0.003) * scale, 0.001, 0, 0, 0, 255);
			for (n = 0; n <= 10; n++) {
				#define SHADOW_OFFSET 0.002
				draw_box_f3(esContext, (x - (w / 2.0) + (w * (float)n / 10.0) - 0.001 + SHADOW_OFFSET) * scale, (y + 0.001 + SHADOW_OFFSET) * scale, 0.001, (x - (w / 2.0) + (w * (float)n / 10.0) + 0.001 + SHADOW_OFFSET) * scale, (y + 0.007 + SHADOW_OFFSET) * scale, 0.001, 2, 2, 2, 255);
				draw_box_f3(esContext, (x - (w / 2.0) + (w * (float)n / 10.0) - 0.001) * scale, (y + 0.001) * scale, 0.001, (x - (w / 2.0) + (w * (float)n / 10.0) + 0.001) * scale, (y + 0.007) * scale, 0.001, 255, 255, 255, 255);
			}
			sprintf(tmp_str, "%s/textures/slider.png", BASE_DIR);
			draw_image_f3(esContext, (x + (value * w / 2000) - 0.03) * scale, (y + 0.01) * scale, (x + (value * w / 2000) + 0.03) * scale, (y + 0.05 - 0.01) * scale, 0.02, tmp_str);
#else
			draw_line_f3(esContext, (x - (w / 2) + w / 2.0 + (value2 * w / 2000)) * scale, (y - 0.001) * scale, 0.001, (x - (w / 2) + w / 2.0 + (value2 * w / 2000)) * scale, (y + 0.05 + 0.001) * scale, 0.001, 127, 127, 127, 255);
			draw_line_f3(esContext, (x - (w / 2) + w / 2.0) * scale, (y - 0.001) * scale, 0.001, (x - (w / 2) + w / 2.0) * scale, (y + 0.05 + 0.001) * scale, 0.001, 127, 127, 127, 255);
#endif
		}
	}
	sprintf(tmp_str, "slider_%s", name);
	set_button(tmp_str, setup.view_mode, (x - (w / 2) - 0.032) * scale, (y) * scale, (x - (w / 2) + w + 0.032) * scale, (y + 0.05) * scale, rcflow_slider_move, (float)0, 1);
}

void rcflow_draw_vplugin (ESContext *esContext, uint8_t id, uint8_t vnum) {
	uint8_t output = 0;
	char tmp_str[64];
	uint8_t num_max = rcflow_plugin_max_get(id);
	// Plugin
	if (RcPlugin[id].type == RCFLOW_PLUGIN_MULTIPLEX_IN) {
		num_max = 3;
		sprintf(tmp_str, "Select:%i", RcPluginEmbedded[id].input[MAX_INPUTS - 1].value + 1);
		draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str, FONT_GREEN, RcPlugin[id].x_virt, RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE + 0.04, 0.002, 0.04, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_title_edit, (float)id);
	}

#ifndef SIMPLE_DRAW
	if (RcPlugin[id].type == RCFLOW_PLUGIN_INPUT) {
		draw_box_f3c2(esContext, RcPlugin[id].x_virt - (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, RcPlugin[id].x_virt + (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt + num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, 55, 55, 55, 127, 100, 255, 100, 120);
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_OUTPUT) {
		draw_box_f3c2(esContext, RcPlugin[id].x_virt - (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, RcPlugin[id].x_virt + (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt + num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, 55, 55, 55, 127, 255, 100, 100, 120);
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_ADC || RcPlugin[id].type == RCFLOW_PLUGIN_SW || RcPlugin[id].type == RCFLOW_PLUGIN_ENC) {
		draw_box_f3c2(esContext, RcPlugin[id].x_virt - (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, RcPlugin[id].x_virt + (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt + num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, 55, 55, 55, 127, 255, 255, 100, 120);
	} else {
		draw_box_f3c2(esContext, RcPlugin[id].x_virt - (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, RcPlugin[id].x_virt + (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt + num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, 55, 55, 55, 127, 100, 100, 255, 120);
	}
#else
	draw_box_f3(esContext, RcPlugin[id].x_virt - (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, RcPlugin[id].x_virt + (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt + num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, 100, 100, 100, 255);
#endif
	draw_rect_f3(esContext, RcPlugin[id].x_virt - (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, RcPlugin[id].x_virt + (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt + num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.001, 200, 200, 200, 155);
	if (lock == 0 || del_plugin == 1) {
		set_button(RcPlugin[id].name, setup.view_mode, RcPlugin[id].x_virt - (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, RcPlugin[id].x_virt + (PLUGIN_WIDTH / 2.0), RcPlugin[id].y_virt + num_max / 2 * LINK_SPACE - TITLE_HEIGHT, rcflow_plugin_move, (float)id, 1);
	}
	sprintf(tmp_str, "name_%s", RcPlugin[id].name);
	if (RcPlugin[id].title[0] != 0) {
		draw_text_button(esContext, tmp_str, setup.view_mode, RcPlugin[id].title, FONT_GREEN, RcPlugin[id].x_virt, RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.002, 0.04, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_title_edit, (float)id);
	} else {
		draw_text_button(esContext, tmp_str, setup.view_mode, RcPlugin[id].name, FONT_GREEN, RcPlugin[id].x_virt, RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE - TITLE_HEIGHT, 0.002, 0.04, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_title_edit, (float)id);
	}

	// Output-Sliders
	if (RcPlugin[id].type == RCFLOW_PLUGIN_ADC || RcPlugin[id].type == RCFLOW_PLUGIN_SW || RcPlugin[id].type == RCFLOW_PLUGIN_ENC || RcPlugin[id].type == RCFLOW_PLUGIN_VADC || RcPlugin[id].type == RCFLOW_PLUGIN_VSW || RcPlugin[id].vview == 1) {
		for (output = 0; output < MAX_OUTPUTS; output++) {
			if (RcPlugin[id].output[output].name[0] != 0 && RcPlugin[id].output[output].type != RCFLOW_INOUT_TYPE_TEMP) {
				sprintf(tmp_str, "%s;%s", RcPlugin[id].name, RcPlugin[id].output[output].name);
				if (RcPlugin[id].output[output].type == RCFLOW_INOUT_TYPE_ONOFF) {
					rcflow_draw_switch(esContext, tmp_str, RcPlugin[id].x_virt, RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.02, SLIDER_WIDTH, RcPlugin[id].output[output].value, RcPlugin[id].output[output].type);
				} else {
					rcflow_draw_slider(esContext, tmp_str, RcPlugin[id].x_virt, RcPlugin[id].y_virt - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.02, SLIDER_WIDTH, RcPlugin[id].output[output].value, 0.0, RcPlugin[id].output[output].type);
				}
			}
		}
	}
}

static int rcflow_sript_view_flag = -1;
static char rcflow_sript_view_text[2048];

static uint8_t rcflow_sript_view (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	if (rcflow_sript_view_flag == -1) {
		rcflow_sript_view_flag = (int)data;
	} else {
		rcflow_sript_view_flag = -1;
	}
	if (rcflow_sript_view_flag != -1) {
		strcpy(rcflow_sript_view_text, RcPlugin[rcflow_sript_view_flag].text);
	}

	return 0;
}

static uint8_t rcflow_sript_view_save (char *name, float x, float y, int8_t button, float data, uint8_t action) {
	if (button == 5) {
		if (scale > SCALE_MIN) {
			scale -= 0.1;
		}
		return 0;
	} else if (button == 4) {
		if (scale < SCALE_MAX) {
			scale += 0.1;
		}
		return 0;
	} else if (button == 2) {
		scale = 1.0;
		return 0;
	}
	if (rcflow_sript_view_flag != -1) {
		strcpy(RcPlugin[rcflow_sript_view_flag].text, rcflow_sript_view_text);
		rcflow_sript_view_flag = -1;
	}
	return 0;
}


void rcflow_draw_plugin (ESContext *esContext, uint8_t id) {
	uint8_t input = 0;
	uint8_t output = 0;
	char tmp_str[64];
	uint8_t num_max = rcflow_plugin_max_get(id);

	// Plugin
#ifndef SIMPLE_DRAW
	// Background
/*
	if (RcPlugin[id].open == 1) {
		if (RcPlugin[id].type == RCFLOW_PLUGIN_INPUT) {
			draw_box_f3c2(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 55, 55, 55, 127, 100, 255, 100, 120);
		} else if (RcPlugin[id].type == RCFLOW_PLUGIN_OUTPUT) {
			draw_box_f3c2(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 55, 55, 55, 127, 255, 100, 100, 120);
		} else if (RcPlugin[id].type == RCFLOW_PLUGIN_ADC || RcPlugin[id].type == RCFLOW_PLUGIN_SW || RcPlugin[id].type == RCFLOW_PLUGIN_ENC) {
			draw_box_f3c2(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 55, 55, 55, 127, 255, 255, 100, 120);
		} else {
			draw_box_f3c2(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 55, 55, 55, 127, 100, 100, 255, 120);
		}
	}
*/

//	draw_box_f3(esContext, PLUGIN_X1 - (LINK_WIDTH - 0.01) * scale, PLUGIN_Y1 + (0.01 * scale), 0.001, PLUGIN_X2 + (LINK_WIDTH + 0.01) * scale, PLUGIN_Y2 + (0.01 * scale), 0.001, 0, 0, 0, 190);

	if (RcPlugin[id].open == 1) {
		draw_box_f3c2(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 50, 50, 50, 255, 90, 90, 90, 255);
		draw_rect_f3(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 0, 0, 0, 255);
	}

	// Title-Bar
	draw_box_f3c2(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.0011, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y1 + (TITLE_HEIGHT / 2.0 * scale), 0.0011, 5, 5, 15, 255, 45, 45, 45, 255);
	draw_box_f3c2(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1 + (TITLE_HEIGHT / 2.0 * scale), 0.0011, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y1 + (TITLE_HEIGHT * scale), 0.0011, 45, 45, 45, 255, 5, 5, 15, 255);
	draw_line_f3(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1 + (TITLE_HEIGHT * scale), 0.0011, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y1 + (TITLE_HEIGHT * scale), 0.001, 10, 10, 10, 255);
#else
	if (RcPlugin[id].open == 1) {
		draw_box_f3(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 100, 100, 100, 255);
	}
	draw_line_f3(esContext, PLUGIN_X1, PLUGIN_Y1 + (TITLE_HEIGHT * scale), 0.0011, PLUGIN_X2, PLUGIN_Y1 + (TITLE_HEIGHT * scale), 0.001, 10, 10, 10, 255);
#endif

	if (id == rcflow_plugin_move_id && rcflow_plugin_move_timer > 0) {
		draw_rect_f3(esContext, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale), PLUGIN_Y2, 0.001, 255, 0, 0, rcflow_plugin_move_timer * 3);
		rcflow_plugin_move_timer--;
	}

	if (lock == 0 || del_plugin == 1) {
		set_button(RcPlugin[id].name, setup.view_mode, PLUGIN_X1 - (LINK_WIDTH * scale), PLUGIN_Y1, PLUGIN_X2 + (LINK_WIDTH * scale), (RcPlugin[id].y + num_max / 2 * LINK_SPACE - TITLE_HEIGHT + canvas_y) * scale, rcflow_plugin_move, (float)id, 1);
	}

	if (RcPlugin[id].open == 1) {
		sprintf(tmp_str, "%s/textures/icon_up.png", BASE_DIR);
	} else {
		sprintf(tmp_str, "%s/textures/icon_down.png", BASE_DIR);
	}
	draw_image_f3(esContext, PLUGIN_X2 + (LINK_WIDTH * scale) - (0.01 + TITLE_HEIGHT - 0.02) * scale, PLUGIN_Y1 + 0.01 * scale, PLUGIN_X2 + (LINK_WIDTH * scale) - 0.01 * scale, PLUGIN_Y1 + (0.01 + TITLE_HEIGHT - 0.02) * scale, 0.002, tmp_str);
	sprintf(tmp_str, "open_%i", id);
	set_button(tmp_str, setup.view_mode, PLUGIN_X2 + (LINK_WIDTH * scale) - (0.01 + TITLE_HEIGHT - 0.02) * scale, PLUGIN_Y1 + 0.01 * scale, PLUGIN_X2 + (LINK_WIDTH * scale) - 0.01 * scale, PLUGIN_Y1 + (0.01 + TITLE_HEIGHT - 0.02) * scale, rcflow_open, (float)id, 0);

	draw_line_f3(esContext, PLUGIN_X2 + (LINK_WIDTH * scale) - (TITLE_HEIGHT) * scale, PLUGIN_Y1, 0.001, PLUGIN_X2 + (LINK_WIDTH * scale) - (TITLE_HEIGHT) * scale, PLUGIN_Y1 + TITLE_HEIGHT * scale, 0.001, 10, 10, 10, 255);



//	draw_rect_f3(esContext, PLUGIN_X1, PLUGIN_Y1, 0.001, PLUGIN_X2, PLUGIN_Y2, 0.001, 200, 200, 200, 155);


	sprintf(tmp_str, "name_%s", RcPlugin[id].name);
	if (RcPlugin[id].title[0] != 0) {
		draw_text_button(esContext, tmp_str, setup.view_mode, RcPlugin[id].title, FONT_GREEN, PLUGIN_X1 - (LINK_WIDTH - 0.01) * scale, PLUGIN_Y1, 0.002, 0.04 * scale, ALIGN_LEFT, ALIGN_TOP, rcflow_plugin_title_edit, (float)id);
	} else {
		draw_text_button(esContext, tmp_str, setup.view_mode, RcPlugin[id].name, FONT_GREEN, PLUGIN_X1 - (LINK_WIDTH - 0.01) * scale, PLUGIN_Y1, 0.002, 0.04 * scale, ALIGN_LEFT, ALIGN_TOP, rcflow_plugin_title_edit, (float)id);
	}

	if (RcPlugin[id].open == 0) {
		return;
	}

	// Inputs
	for (input = 0; input < MAX_INPUTS; input++) {
		if (RcPlugin[id].input[input].name[0] != 0 && RcPlugin[id].input[input].type != RCFLOW_INOUT_TYPE_TEMP) {
			sprintf(tmp_str, "%s/textures/conn.png", BASE_DIR);
			draw_image_f3(esContext, PLUGIN_X1 - (LINK_WIDTH - 0.025 + CONN_RADIUS) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + (LINK_SPACE / 2.0) - CONN_RADIUS + canvas_y) * scale, PLUGIN_X1 - (LINK_WIDTH - 0.025 - CONN_RADIUS) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + (LINK_SPACE / 2.0) + CONN_RADIUS + canvas_y) * scale, 0.02, tmp_str);

			sprintf(tmp_str, "%s;%s", RcPlugin[id].name, RcPlugin[id].input[input].name);
#ifndef SIMPLE_DRAW
			if (strcmp(link_select_to, tmp_str) == 0) {
				draw_box_f3(esContext, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, 0.001, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 0, 0, 125);
//			} else {
//				draw_box_f3(esContext, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, 0.001, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 100, 100, 100, 125);
			}
//			draw_rect_f3(esContext, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, 0.001, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 255, 255, 125);
#else
			if (strcmp(link_select_to, tmp_str) == 0) {
				draw_box_f3(esContext, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, 0.001, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 0, 0, 255);
			} else {
				draw_box_f3(esContext, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, 0.001, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 100, 100, 100, 255);
			}
#endif
			if (RcPlugin[id].type == RCFLOW_PLUGIN_MULTIPLEX_OUT) {
				uint8_t select = 0;
				int16_t range_begin;
				int16_t range_end;
				int16_t n = 0;
				for (n = 0; n < 7; n++) {
					range_begin = n * 2000 / 6 - 1000;
					range_end = (n + 1) * 2000 / 6 - 1000;
					if (RcPlugin[id].input[6].value >= range_begin && RcPlugin[id].input[6].value <= range_end && n < 6) {
						select = n;
						break;
					}
				}
				if (select == input) {
					draw_rect_f3(esContext, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, 0.001, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 0, 0, 255);
				}
			}
			set_button(tmp_str, setup.view_mode, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.1 + canvas_y) * scale, rcflow_link_mark, (float)1, 0);
			draw_text_button(esContext, "#", setup.view_mode, RcPlugin[id].input[input].name, FONT_GREEN, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y) * scale, 0.002, 0.03 * scale, ALIGN_RIGHT, ALIGN_TOP, rcflow_null, 0.0);
#ifndef SIMPLE_DRAW
			if (RcPlugin[id].input[input].type == RCFLOW_INOUT_TYPE_LINEAR) {
				sprintf(tmp_str, "%i", (RcPlugin[id].input[input].value + 1000) / 2);
			} else {
				sprintf(tmp_str, "%i", RcPlugin[id].input[input].value);
			}
//			draw_text_f3(esContext, (RcPlugin[id].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y + 0.04) * scale, 0.005, 0.03 * scale, 0.03 * scale, FONT_GREEN, tmp_str);


	draw_text_button(esContext, "#", setup.view_mode, tmp_str, FONT_GREEN, PLUGIN_X1, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + canvas_y + 0.04) * scale, 0.002, 0.03 * scale, ALIGN_RIGHT, ALIGN_TOP, rcflow_null, 0.0);

#endif
		}
	}
	// Outputs
	for (output = 0; output < MAX_OUTPUTS; output++) {
		if (RcPlugin[id].output[output].name[0] != 0 && RcPlugin[id].output[output].type != RCFLOW_INOUT_TYPE_TEMP) {


			sprintf(tmp_str, "%s/textures/conn.png", BASE_DIR);
			draw_image_f3(esContext, PLUGIN_X2 + (LINK_WIDTH - 0.025 - CONN_RADIUS) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + (LINK_SPACE / 2.0) - CONN_RADIUS + canvas_y) * scale, PLUGIN_X2 + (LINK_WIDTH - 0.025 + CONN_RADIUS) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + (LINK_SPACE / 2.0) + CONN_RADIUS + canvas_y) * scale, 0.02, tmp_str);


			sprintf(tmp_str, "%s;%s", RcPlugin[id].name, RcPlugin[id].output[output].name);

#ifndef SIMPLE_DRAW
			if (strcmp(link_select_from, tmp_str) == 0) {
				draw_box_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, 0.001, (RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 0, 0, 125);
//			} else {
//				draw_box_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, 0.001, (RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 100, 100, 100, 125);
			}
//			draw_rect_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, 0.001, (RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 255, 255, 125);
#else
			if (strcmp(link_select_from, tmp_str) == 0) {
				draw_box_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, 0.001, (RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 0, 0, 255);
			} else {
				draw_box_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, 0.001, (RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 100, 100, 100, 255);
			}
#endif
			if (RcPlugin[id].type == RCFLOW_PLUGIN_MULTIPLEX_IN) {
				uint8_t select = 0;
				int16_t range_begin;
				int16_t range_end;
				int16_t n = 0;
				for (n = 0; n < 7; n++) {
					range_begin = n * 2000 / 6 - 1000;
					range_end = (n + 1) * 2000 / 6 - 1000;
					if (RcPlugin[id].input[1].value >= range_begin && RcPlugin[id].input[1].value <= range_end && n < 6) {
						select = n;
						break;
					}
				}
				if (select == output) {
					draw_rect_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, 0.001, (RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.07 + canvas_y) * scale, 0.001, 255, 0, 0, 255);
				}
			}
			set_button(tmp_str, setup.view_mode, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, (RcPlugin[id].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.1 + canvas_y) * scale, rcflow_link_mark, (float)0, 0);
			draw_text_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + canvas_y) * scale, 0.005, 0.03 * scale, 0.03 * scale, FONT_GREEN, RcPlugin[id].output[output].name);
#ifndef SIMPLE_DRAW
			sprintf(tmp_str, "%i", RcPlugin[id].output[output].value);
			sprintf(tmp_str, "%i", RcPlugin[id].output[output].value);
			draw_text_f3(esContext, PLUGIN_X2, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.04 + canvas_y) * scale, 0.005, 0.03 * scale, 0.03 * scale, FONT_GREEN, tmp_str);
#endif
		}
	}


	if (del_plugin == 1) {
		return;
	}


	if (RcPlugin[id].type == RCFLOW_PLUGIN_INPUT) {
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_OUTPUT) {
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_ATV) {
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_LIMIT) {
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_MIXER) {
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_TCL) {
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_CURVE) {
		sprintf(tmp_str, "preset_%s", RcPlugin[id].name);
		draw_text_button(esContext, tmp_str, setup.view_mode, "PRESET", FONT_GREEN, (RcPlugin[id].x + canvas_x) * scale, PLUGIN_Y2 - 0.05 * scale, 0.002, 0.04 * scale, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_curve_preset, (float)id);
	}
	// Input-Sliders
	if (RcPlugin[id].type != RCFLOW_PLUGIN_TCL) {
		for (input = 0; input < MAX_INPUTS; input++) {
			if (RcPlugin[id].input[input].name[0] != 0 && RcPlugin[id].input[input].type != RCFLOW_INOUT_TYPE_TEMP) {
				sprintf(tmp_str, "%s;%s", RcPlugin[id].name, RcPlugin[id].input[input].name);
				if (RcPlugin[id].input[input].type == RCFLOW_INOUT_TYPE_ONOFF) {
					rcflow_draw_switch(esContext, tmp_str, RcPlugin[id].x + canvas_x, RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.02 + canvas_y, SLIDER_WIDTH, RcPlugin[id].input[input].value, RcPlugin[id].input[input].type);
				} else if (RcPlugin[id].type == RCFLOW_PLUGIN_MULTIVALUE && input == 1) {
					sprintf(tmp_str, "mode: %i", RcPlugin[id].option + 1);
					draw_text_f3(esContext, (RcPlugin[id].x - 0.1 + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.02 + canvas_y) * scale, 0.005, 0.04 * scale, 0.04 * scale, FONT_GREEN, tmp_str);
				} else if (RcPlugin[id].input[input].type == RCFLOW_INOUT_TYPE_TEXT) {
					draw_text_f3(esContext, (RcPlugin[id].x - 0.1 + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.02 + canvas_y) * scale, 0.005, 0.03 * scale, 0.03 * scale, FONT_GREEN, RcPlugin[id].input[input].tmp_text);
				} else {
					if (input == 0) {
						rcflow_draw_slider(esContext, tmp_str, RcPlugin[id].x + canvas_x, RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.02 + canvas_y, SLIDER_WIDTH, RcPlugin[id].input[input].value, RcPlugin[id].output[0].value, RcPlugin[id].input[input].type);
					} else {
						rcflow_draw_slider(esContext, tmp_str, RcPlugin[id].x + canvas_x, RcPlugin[id].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + 0.02 + canvas_y, SLIDER_WIDTH, RcPlugin[id].input[input].value, 0.0, RcPlugin[id].input[input].type);
					}
				}
			}
		}
	}
	if (RcPlugin[id].type == RCFLOW_PLUGIN_TCL) {
		sprintf(tmp_str, "script_view_%s", RcPlugin[id].name);
		draw_text_button(esContext, tmp_str, setup.view_mode, "[SCRIPT]", FONT_GREEN, RcPlugin[id].x + canvas_x - 0.1, RcPlugin[id].y - num_max / 2 * LINK_SPACE + 1.0 * LINK_SPACE + 0.04 + canvas_y, 0.002, 0.04 * scale, ALIGN_LEFT, ALIGN_TOP, rcflow_sript_view, (float)id);
	} else if (RcPlugin[id].type == RCFLOW_PLUGIN_CURVE) {
		float pos = 2.0 + ((float)(RcPlugin[id].input[0].value + 1000) * (MAX_CURVE_POINTS - 1) / 2000.0);
		float pos_out = ((float)(RcPlugin[id].output[0].value) * LINK_SPACE / 1000.0);
		float pos2 = 2.0 + ((float)(RcPlugin[id].input[1].value + 1000) * (MAX_CURVE_POINTS - 1) / 2000.0);
		float pos2_out = ((float)(RcPlugin[id].output[1].value) * LINK_SPACE / 1000.0);
#ifndef SIMPLE_DRAW
		draw_box_f3c2(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 200, 200, 200, 200, 100, 100, 100, 200);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y + 0.04 + canvas_y) * scale, 0.001, 190, 190, 190, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 190, 190, 190, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + pos * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + pos * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 55, 255, 255, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x + pos_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + pos_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 55, 255, 255, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + pos2 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + pos2 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 255, 55, 255, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x + pos2_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + pos2_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 255, 55, 255, 255);
		draw_rect_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 55, 255, 255, 255);
#else
		draw_box_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 100, 100, 255, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y + 0.04 + canvas_y) * scale, 0.001, 190, 190, 190, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 190, 190, 190, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + pos * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + pos * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 55, 255, 255, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x + pos_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + pos_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 55, 255, 255, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x - 0.1) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + pos2 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + 0.1) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + pos2 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 255, 55, 255, 255);
		draw_line_f3(esContext, (RcPlugin[id].x + canvas_x + pos2_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + 2.0 * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + pos2_out) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + ((float)MAX_CURVE_POINTS + 1.0) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 255, 55, 255, 255);
#endif
		float pos_out2;
		float pos_out3;
		uint8_t n = 0;
		for (n = 0; n < MAX_CURVE_POINTS - 1; n++) {
			pos_out2 = ((float)(RcPlugin[id].input[n + 2].value) * LINK_SPACE / 1000.0);
			pos_out3 = ((float)(RcPlugin[id].input[n + 3].value) * LINK_SPACE / 1000.0);
			draw_line_f3(esContext, (RcPlugin[id].x + canvas_x + pos_out2) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + (n + 2) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (RcPlugin[id].x + canvas_x + pos_out3) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + (n + 3) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, 0, 0, 0, 255);
#ifndef SIMPLE_DRAW
			draw_circleFilled_f3(esContext, (RcPlugin[id].x + canvas_x + pos_out2) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + (n + 2) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (0.01) * scale, 0, 0, 0, 127);
#else
			draw_box_f3(esContext, (RcPlugin[id].x + canvas_x + pos_out2 - 0.01) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + (n + 2) * LINK_SPACE + 0.04 + canvas_y - 0.01) * scale, 0.001, (RcPlugin[id].x + canvas_x + pos_out2 + 0.01) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + (n + 2) * LINK_SPACE + 0.04 + canvas_y + 0.01) * scale, 0.001, 10, 10, 10, 255);
#endif
		}
		pos_out2 = ((float)(RcPlugin[id].input[n + 2].value) * LINK_SPACE / 1000.0);
#ifndef SIMPLE_DRAW
		draw_circleFilled_f3(esContext, (RcPlugin[id].x + canvas_x + pos_out2) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + (n + 2) * LINK_SPACE + 0.04 + canvas_y) * scale, 0.001, (0.01) * scale, 0, 0, 0, 127);
#else
		draw_box_f3(esContext, (RcPlugin[id].x + canvas_x + pos_out2 - 0.01) * scale, (RcPlugin[id].y - num_max / 2.0 * LINK_SPACE + (n + 2) * LINK_SPACE + 0.04 + canvas_y - 0.01) * scale, 0.001, (RcPlugin[id].x + canvas_x + pos_out2 + 0.01) * scale, (RcPlugin[id].y - num_max / 2 * LINK_SPACE + (n + 2) * LINK_SPACE + 0.04 + canvas_y + 0.01) * scale, 0.001, 10, 10, 10, 255);
#endif
	}
	// Output-Sliders
	if (RcPlugin[id].type == RCFLOW_PLUGIN_ADC || RcPlugin[id].type == RCFLOW_PLUGIN_SW || RcPlugin[id].type == RCFLOW_PLUGIN_ENC || RcPlugin[id].type == RCFLOW_PLUGIN_VSW || RcPlugin[id].type == RCFLOW_PLUGIN_VADC || RcPlugin[id].type == RCFLOW_PLUGIN_MULTIPLEX_IN || RcPlugin[id].vview == 1) {
		for (output = 0; output < MAX_OUTPUTS; output++) {
			sprintf(tmp_str, "%s;%s", RcPlugin[id].name, RcPlugin[id].output[output].name);
			if (RcPlugin[id].output[output].name[0] != 0 && RcPlugin[id].output[output].type != RCFLOW_INOUT_TYPE_TEMP) {
				if (RcPlugin[id].output[output].type == RCFLOW_INOUT_TYPE_ONOFF) {
					rcflow_draw_switch(esContext, tmp_str, RcPlugin[id].x + canvas_x, RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.02 + canvas_y, SLIDER_WIDTH, RcPlugin[id].output[output].value, RcPlugin[id].output[output].type);
				} else {
					rcflow_draw_slider(esContext, tmp_str, RcPlugin[id].x + canvas_x, RcPlugin[id].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + 0.02 + canvas_y, SLIDER_WIDTH, RcPlugin[id].output[output].value, 0.0, RcPlugin[id].output[output].type);
				}
			}
		}
	}
}


#ifndef SIMPLE_DRAW
float X[7];
float Y[7];
void draw_spline (ESContext *esContext, int start, int count, int r, int g, int b, int a) {
	int i = 0;
	int j = 0;
	float t = 0;
	float Cx[4] = {0,0,0,0};
	float Cy[4] = {0,0,0,0};
	float Matrix[4][4] = {
		{ 1.0,   4.0,   1.0,  0.0},
		{-3.0,   0.0,   3.0,  0.0},
		{ 3.0,  -6.0,   3.0,  0.0},
		{-1.0,   3.0,  -3.0,  1.0}
	};
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			Cx[i] += Matrix[i][j] * X[j + start] / 6.0;
			Cy[i] += Matrix[i][j] * Y[j + start] / 6.0;
		}
	}
	float last_x = 0.0;
	float last_y = 0.0;
	float step = 1.0 / count;
	for (t = 0; t <= 1; t += step) {
		float x = Cx[0] + t * (Cx[1] + t * (Cx[2] + t * Cx[3]));
		float y = Cy[0] + t * (Cy[1] + t * (Cy[2] + t * Cy[3]));
		if (t != 0) {
			draw_line_f3(esContext, last_x, last_y, 0.005, x, y, 0.005, r, g, b, a);
		}
		last_x = x;
		last_y = y;
	}
}
#endif

void rcflow_draw_link (ESContext *esContext, uint8_t id) {
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	uint8_t flag1 = 0;
	uint8_t flag2 = 0;
	float a_x = 0.0;
	float a_y = 0.0;
	float b_x = 0.0;
	float b_y = 0.0;
	float color = 0.0;
	char tmp_str[64];
	if (RcLink[id].from[0] != 0 && RcLink[id].to[0] != 0) {
		for (plugin = 0; plugin < MAX_PLUGINS && (flag1 == 0 || flag2 == 0); plugin++) {
			if (RcPlugin[plugin].name[0] != 0) {
				for (output = 0; output < MAX_OUTPUTS && flag1 == 0; output++) {
					if (RcPlugin[plugin].output[output].name[0] != 0) {
						sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].output[output].name);
						if (strcmp(RcLink[id].from, tmp_str) == 0) {
							uint8_t num_max = rcflow_plugin_max_get(plugin);
							a_x = RcPlugin[plugin].x + (PLUGIN_WIDTH / 2.0) + LINK_WIDTH - 0.025 + canvas_x;
							if (RcPlugin[plugin].open == 1) {
								a_y = RcPlugin[plugin].y - num_max / 2 * LINK_SPACE + output * LINK_SPACE + (LINK_SPACE / 2.0) + canvas_y;
								flag1 = 1;
							} else {
								a_y = RcPlugin[plugin].y - num_max / 2 * LINK_SPACE - (TITLE_HEIGHT / 2.0) + canvas_y;
								flag1 = 2;
							}
							color = RcPlugin[plugin].output[output].value / 10.0;
							break;
						}
					}
				}
				for (input = 0; input < MAX_INPUTS && flag2 == 0; input++) {
					if (RcPlugin[plugin].input[input].name[0] != 0) {
						sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].input[input].name);
						if (strcmp(RcLink[id].to, tmp_str) == 0) {
							uint8_t num_max = rcflow_plugin_max_get(plugin);
							b_x = RcPlugin[plugin].x - (PLUGIN_WIDTH / 2.0) - LINK_WIDTH + 0.025 + canvas_x;
							if (RcPlugin[plugin].open == 1) {
								b_y = RcPlugin[plugin].y - num_max / 2 * LINK_SPACE + input * LINK_SPACE + (LINK_SPACE / 2.0) + canvas_y;
								flag2 = 1;
							} else {
								b_y = RcPlugin[plugin].y - num_max / 2 * LINK_SPACE - (TITLE_HEIGHT / 2.0) + canvas_y;
								flag2 = 2;
							}
							break;
						}
					}
				}
			}
		}
	}
	if (flag1 != 0 && flag2 != 0) {
#ifndef SIMPLE_DRAW
		if (flag1 == 1) {
//			draw_circleFilled_f3(esContext, (a_x) * scale, (a_y) * scale, 0.005, (0.005) * scale, 0, 0, 255, 128);
		}
		if (flag2 == 1) {
//			draw_circleFilled_f3(esContext, (b_x) * scale, (b_y) * scale, 0.005, (0.005) * scale, 0, 0, 255, 128);
		}
		glLineWidth(3);
		int i = 0;
		int n = 0;
		float mid_x = (b_x - a_x) / 2.0 + a_x;
		float mid_y = (b_y - a_y) / 2.0 + a_y;
		float cavity = (b_x - a_x - 0.14) / 50.0;
		if (cavity < 0.0) {
			cavity *= -1;
		}
//		cavity = 0.0;
		X[n] = (a_x - 0.07) * scale;
		Y[n++] = (a_y) * scale;
		X[n] = (a_x) * scale;
		Y[n++] = (a_y) * scale;
		X[n] = (a_x + 0.07) * scale;
		Y[n++] = (a_y + cavity / 3.0) * scale;
		X[n] = (mid_x) * scale;
		Y[n++] = (mid_y + cavity) * scale;
		X[n] = (b_x - 0.07) * scale;
		Y[n++] = (b_y + cavity / 3.0) * scale;
		X[n] = (b_x) * scale;
		Y[n++] = (b_y) * scale;
		X[n] = (b_x + 0.07) * scale;
		Y[n++] = (b_y) * scale;
		for (i = 0; i < n - 3; i++) {
			draw_spline(esContext, i, 100, 100, 100 + (uint8_t)color, 160, 255);
		}
#else
		glLineWidth(3);
		draw_line_f3(esContext, (a_x) * scale, (a_y) * scale, 0.005, (b_x) * scale, (b_y) * scale, 0.005, 100, 100 + (uint8_t)color, 160, 255);
#endif
		glLineWidth(1);

	}
}

void rcflow_link (char *from, char *to) {
	uint8_t link = 0;
	for (link = 0; link < MAX_LINKS; link++) {
		if (RcLink[link].from[0] == 0 || RcLink[link].to[0] == 0) {
			strcpy(RcLink[link].from, from);
			strcpy(RcLink[link].to, to);
			return;
		}
	}
	return;
}

void rcflow_calc_Embedded (void) {
	uint8_t plugin = 0;
#ifdef RPI_NO_X
	uint8_t output = 0;
#endif
	uint8_t link = 0;
	uint8_t n = 0;

	// Zero connected Inputs
	for (link = 0; link < MAX_LINKS; link++) {
		if (RcLinkEmbedded[link].to[0] != 255 && RcLinkEmbedded[link].to[1] != 255) {
			RcPluginEmbedded[RcLinkEmbedded[link].to[0]].input[RcLinkEmbedded[link].to[1]].value = 0.0;
		}
	}

#ifdef ____RPI_NO_X
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_ADC) {
			for (output = 0; output < 8; output++) {
				RcPluginEmbedded[plugin].output[output].value = rctransmitter_input[output] * 10;
				//SDL_Log("rcflow: ## %s (%i) == %i ##\n", RcPlugin[plugin].output[output].name, output, rctransmitter_input[output] * 10);
			}
		}
	}
#endif

#ifdef ADC_TEST
#ifndef RPI_NO_X
	uint8_t output = 0;
#endif
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_ADC) {
			for (output = 0; output < MAX_OUTPUTS; output++) {
				if (RcPluginEmbedded[plugin].output[output].option == 1) {
					RcPluginEmbedded[plugin].output[output].value = adc[output] * -1;
				} else {
					RcPluginEmbedded[plugin].output[output].value = adc[output];
				}
//				SDL_Log("rcflow: ## %s (%i) == %i ##\n", RcPlugin[plugin].output[output].name, output, RcPluginEmbedded[plugin].output[output].value);
			}
		}
	}

	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_SW) {
			for (output = 0; output < MAX_OUTPUTS; output++) {
				if (RcPluginEmbedded[plugin].output[output].option == 1) {
					RcPluginEmbedded[plugin].output[output].value = sw[output] * -1;
				} else {
					RcPluginEmbedded[plugin].output[output].value = sw[output];
				}
//				SDL_Log("rcflow: ## %s (%i) == %i ##\n", RcPlugin[plugin].output[output].name, output, RcPluginEmbedded[plugin].output[output].value);
			}
		}
	}
#endif

	// Link-Calculation
	for (link = 0; link < MAX_LINKS; link++) {
		if (RcLinkEmbedded[link].to[0] != 255 && RcLinkEmbedded[link].to[1] != 255) {
			RcPluginEmbedded[RcLinkEmbedded[link].to[0]].input[RcLinkEmbedded[link].to[1]].value += RcPluginEmbedded[RcLinkEmbedded[link].from[0]].output[RcLinkEmbedded[link].from[1]].value;
			RcPluginEmbedded[RcLinkEmbedded[link].to[0]].input[RcLinkEmbedded[link].to[1]].value = rcflow_value_limit(RcPluginEmbedded[RcLinkEmbedded[link].to[0]].input[RcLinkEmbedded[link].to[1]].value, -1000, 1000);
		}
	}

	// Plugin-Calculation
	for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
		if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_INPUT) {
			RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value + RcPluginEmbedded[plugin].input[1].value / 3.0;
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[1].value, -1000, 1000);
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_OUTPUT) {
			RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value + RcPluginEmbedded[plugin].input[1].value / 3.0;
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[1].value, -1000, 1000);
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_ATV) {
			if (RcPluginEmbedded[plugin].input[3].value > 0) {
				if (RcPluginEmbedded[plugin].input[0].value > 0) {
					RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value * RcPluginEmbedded[plugin].input[1].value / 1000;
				} else if (RcPluginEmbedded[plugin].input[0].value < 0) {
					RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value * RcPluginEmbedded[plugin].input[2].value / -1000;
				} else {
					RcPluginEmbedded[plugin].output[0].value = 0.0;
				}
			} else {
				RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value;
			}
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_BUTTERFLY) {
			RcPluginEmbedded[plugin].output[0].value = 0;
			RcPluginEmbedded[plugin].output[1].value = 0;
			RcPluginEmbedded[plugin].output[2].value = 0;
			RcPluginEmbedded[plugin].output[3].value = 0;
			RcPluginEmbedded[plugin].output[5].value = 0;
			RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value;
			RcPluginEmbedded[plugin].output[1].value = (int16_t)(  (float)RcPluginEmbedded[plugin].input[0].value * 0.6 );
			RcPluginEmbedded[plugin].output[2].value = (int16_t)(  (float)RcPluginEmbedded[plugin].input[0].value * -0.6 );
			RcPluginEmbedded[plugin].output[3].value = RcPluginEmbedded[plugin].input[0].value * -1;
			RcPluginEmbedded[plugin].output[0].value += RcPluginEmbedded[plugin].input[1].value;
			RcPluginEmbedded[plugin].output[1].value += RcPluginEmbedded[plugin].input[1].value;
			RcPluginEmbedded[plugin].output[2].value += RcPluginEmbedded[plugin].input[1].value;
			RcPluginEmbedded[plugin].output[3].value += RcPluginEmbedded[plugin].input[1].value;
			RcPluginEmbedded[plugin].output[0].value += RcPluginEmbedded[plugin].input[3].value;
			RcPluginEmbedded[plugin].output[1].value += (int16_t)(  (float)RcPluginEmbedded[plugin].input[3].value * 0.6 );
			RcPluginEmbedded[plugin].output[2].value += (int16_t)(  (float)RcPluginEmbedded[plugin].input[3].value * -0.6 );
			RcPluginEmbedded[plugin].output[3].value += RcPluginEmbedded[plugin].input[3].value * -1;
			RcPluginEmbedded[plugin].output[5].value += RcPluginEmbedded[plugin].input[3].value;
			RcPluginEmbedded[plugin].output[0].value += RcPluginEmbedded[plugin].input[4].value;
			RcPluginEmbedded[plugin].output[1].value += RcPluginEmbedded[plugin].input[4].value * -1;
			RcPluginEmbedded[plugin].output[2].value += RcPluginEmbedded[plugin].input[4].value * -1;
			RcPluginEmbedded[plugin].output[3].value += RcPluginEmbedded[plugin].input[4].value;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_MULTIPLEX_IN) {
			uint8_t select = 0;
			int16_t range_begin;
			int16_t range_end;
			for (n = 0; n < 7; n++) {
				range_begin = n * 2000 / 6 - 1000;
				range_end = (n + 1) * 2000 / 6 - 1000;
				if (RcPluginEmbedded[plugin].input[1].value >= range_begin && RcPluginEmbedded[plugin].input[1].value <= range_end && n < 6) {
					select = n;
					break;
				}
			}
			if (select != RcPluginEmbedded[plugin].input[MAX_INPUTS - 1].value) {
				for (link = 0; link < MAX_LINKS; link++) {
					if (RcLinkEmbedded[link].to[0] == plugin && RcLinkEmbedded[link].to[1] == 0) {
						RcPluginEmbedded[RcLinkEmbedded[link].from[0]].output[RcLinkEmbedded[link].from[1]].value = RcPluginEmbedded[plugin].output[select].value;
					}
				}
			}
			RcPluginEmbedded[plugin].output[select].value = RcPluginEmbedded[plugin].input[0].value;
			RcPluginEmbedded[plugin].input[MAX_INPUTS - 1].value = select;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_MULTIPLEX_OUT) {
			uint8_t select = 0;
			int16_t range_begin;
			int16_t range_end;
			for (n = 0; n < 7; n++) {
				range_begin = n * 2000 / 6 - 1000;
				range_end = (n + 1) * 2000 / 6 - 1000;
				if (RcPluginEmbedded[plugin].input[6].value >= range_begin && RcPluginEmbedded[plugin].input[6].value <= range_end && n < 6) {
					select = n;
					break;
				}
			}
			RcPluginEmbedded[plugin].option = select;
			if (select != RcPluginEmbedded[plugin].input[MAX_INPUTS - 1].value) {
				for (link = 0; link < MAX_LINKS; link++) {
					if (RcLinkEmbedded[link].to[0] == plugin && RcLinkEmbedded[link].to[1] == select) {
						RcPluginEmbedded[RcLinkEmbedded[link].from[0]].output[RcLinkEmbedded[link].from[1]].value = RcPluginEmbedded[plugin].output[select + 2].value;
					}
				}
			}
			RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[select].value;
			RcPluginEmbedded[plugin].output[select + 2].value = RcPluginEmbedded[plugin].input[select].value;
			RcPluginEmbedded[plugin].input[MAX_INPUTS - 1].value = select;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_MULTIVALUE) {
			uint8_t select = 0;
			int16_t range_begin;
			int16_t range_end;
			for (n = 0; n < 7; n++) {
				range_begin = n * 2000 / 6 - 1000;
				range_end = (n + 1) * 2000 / 6 - 1000;
				if (RcPluginEmbedded[plugin].input[1].value >= range_begin && RcPluginEmbedded[plugin].input[1].value <= range_end && n < 6) {
					select = n;
					break;
				}
			}
			RcPluginEmbedded[plugin].option = select;
			if (select != RcPluginEmbedded[plugin].input[MAX_INPUTS - 1].value) {
				for (link = 0; link < MAX_LINKS; link++) {
					if (RcLinkEmbedded[link].to[0] == plugin && RcLinkEmbedded[link].to[1] == 0) {
						RcPluginEmbedded[RcLinkEmbedded[link].from[0]].output[RcLinkEmbedded[link].from[1]].value = RcPluginEmbedded[plugin].output[select + 2].value;
						RcPluginEmbedded[plugin].input[0].value = RcPluginEmbedded[plugin].output[select + 2].value;
					}
				}
			}
			RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value;
			RcPluginEmbedded[plugin].output[select + 2].value = RcPluginEmbedded[plugin].input[0].value;
			RcPluginEmbedded[plugin].input[MAX_INPUTS - 1].value = select;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_MIXER) {
			if (RcPluginEmbedded[plugin].input[4].value > 0) {
				RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit((RcPluginEmbedded[plugin].input[0].value * RcPluginEmbedded[plugin].input[1].value / 1000) + (RcPluginEmbedded[plugin].input[2].value * RcPluginEmbedded[plugin].input[3].value / 1000), -1000, 1000);
			} else if (RcPluginEmbedded[plugin].input[5].value > 0) {
				RcPluginEmbedded[plugin].output[0].value = 0;
			}
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_LIMIT) {
			if (RcPluginEmbedded[plugin].input[3].value > 0) {
				RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].input[0].value, RcPluginEmbedded[plugin].input[2].value, RcPluginEmbedded[plugin].input[1].value);
			} else {
				RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value;
			}
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_DELAY) {
			if (RcPluginEmbedded[plugin].input[3].value > 500) {
				float val_up = rcflow_value_limit((RcPluginEmbedded[plugin].input[1].value + 1000) / 2.0, 0.0, 1000);
				float val_down = rcflow_value_limit((RcPluginEmbedded[plugin].input[2].value + 1000) / 2.0, 0.0, 1000);
				if (RcPluginEmbedded[plugin].output[0].value > RcPluginEmbedded[plugin].input[0].value + val_up) {
					RcPluginEmbedded[plugin].output[0].value -= val_up;
				} else if (RcPluginEmbedded[plugin].output[0].value < RcPluginEmbedded[plugin].input[0].value - val_down) {
					RcPluginEmbedded[plugin].output[0].value += val_down;
				} else {
					RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value;
				}
			} else {
				RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_CURVE) {
			int16_t range_begin;
			int16_t range_end;
			float range_pos;
			for (n = 0; n < MAX_CURVE_POINTS; n++) {
				range_begin = (n) * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
				range_end = (n + 1) * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
				if (RcPluginEmbedded[plugin].input[0].value >= range_begin && RcPluginEmbedded[plugin].input[0].value <= range_end && n < (MAX_CURVE_POINTS - 1)) {
					range_pos = (float)(RcPluginEmbedded[plugin].input[0].value - range_begin) * 1.0 / (float)(range_end - range_begin);
					RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[n + 2].value + (int16_t)((float)(RcPluginEmbedded[plugin].input[n + 3].value - RcPluginEmbedded[plugin].input[n + 2].value) * range_pos);
					break;
				}
			}
			for (n = 0; n < MAX_CURVE_POINTS; n++) {
				range_begin = (n) * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
				range_end = (n + 1) * 2000 / (MAX_CURVE_POINTS - 1) - 1000;
				if (RcPluginEmbedded[plugin].input[1].value >= range_begin && RcPluginEmbedded[plugin].input[1].value <= range_end && n < (MAX_CURVE_POINTS - 1)) {
					range_pos = (float)(RcPluginEmbedded[plugin].input[1].value - range_begin) * 1.0 / (float)(range_end - range_begin);
					RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].input[n + 2].value + (int16_t)((float)(RcPluginEmbedded[plugin].input[n + 3].value - RcPluginEmbedded[plugin].input[n + 2].value) * range_pos);
					break;
				}
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[1].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[2].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
			RcPluginEmbedded[plugin].output[3].value = RcPluginEmbedded[plugin].output[1].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_TCL) {
			if (rcflow_tcl_startup == 0) {
				rcflow_tcl_init();
			}

			// set inital variables for tcl (input/output values)
			char tmp_str[1024];
			for (n = 0; n < MAX_INPUTS; n++) {
				sprintf(tmp_str, "set Input(%i) \"%i\"", n, RcPluginEmbedded[plugin].input[n].value);
//				Tcl_Eval(rcflow_tcl_interp, tmp_str);
			}
			for (n = 0; n < MAX_OUTPUTS; n++) {
				sprintf(tmp_str, "set Output(%i) \"%i\"", n, RcPluginEmbedded[plugin].output[n].value);
//				Tcl_Eval(rcflow_tcl_interp, tmp_str);
			}

			// eval plugin-script
//			rcflow_tcl_run(RcPlugin[plugin].text);

			// update output values
			for (n = 0; n < MAX_OUTPUTS; n++) {
				sprintf(tmp_str, "set_output %i %i $Output(%i)", plugin, n, n);
//				Tcl_Eval(rcflow_tcl_interp, tmp_str);
			}

		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_H120) {
			// Reset
			RcPluginEmbedded[plugin].output[0].value = 0;
			RcPluginEmbedded[plugin].output[1].value = 0;
			RcPluginEmbedded[plugin].output[2].value = 0;
			// Roll
			RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value / 2;
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].input[0].value / -2;
			// Nick
			RcPluginEmbedded[plugin].output[0].value -= RcPluginEmbedded[plugin].input[1].value / 2;
			RcPluginEmbedded[plugin].output[1].value -= RcPluginEmbedded[plugin].input[1].value / 2;
			RcPluginEmbedded[plugin].output[2].value += RcPluginEmbedded[plugin].input[1].value;
			// Pitch
			RcPluginEmbedded[plugin].output[0].value += RcPluginEmbedded[plugin].input[2].value;
			RcPluginEmbedded[plugin].output[1].value += RcPluginEmbedded[plugin].input[2].value;
			RcPluginEmbedded[plugin].output[2].value += RcPluginEmbedded[plugin].input[2].value;
			// Limits
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[1].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[2].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[2].value, -1000, 1000);
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_SWITCH) {
			if (RcPluginEmbedded[plugin].input[2].value > 0) {
				RcPluginEmbedded[plugin].output[0].value = 0;
				RcPluginEmbedded[plugin].output[1].value = 0;
				RcPluginEmbedded[plugin].output[2].value = 0;
				RcPluginEmbedded[plugin].output[3].value = 0;
			}
			if (RcPluginEmbedded[plugin].input[3].value < 500) {
				RcPluginEmbedded[plugin].output[0].value = RcPluginEmbedded[plugin].input[0].value;
				RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].input[1].value;
			} else if (RcPluginEmbedded[plugin].input[3].value > 500) {
				RcPluginEmbedded[plugin].output[2].value = RcPluginEmbedded[plugin].input[0].value;
				RcPluginEmbedded[plugin].output[3].value = RcPluginEmbedded[plugin].input[1].value;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[1].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[2].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[2].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[3].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[3].value, -1000, 1000);
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_RANGESW) {
			if (RcPluginEmbedded[plugin].input[0].value >= RcPluginEmbedded[plugin].input[1].value && RcPluginEmbedded[plugin].input[0].value <= RcPluginEmbedded[plugin].input[2].value) {
				RcPluginEmbedded[plugin].output[0].value = 1000;
			} else {
				RcPluginEmbedded[plugin].output[0].value = -1000;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_ALARM) {
			if (RcPluginEmbedded[plugin].input[0].value >= RcPluginEmbedded[plugin].input[1].value && RcPluginEmbedded[plugin].input[0].value <= RcPluginEmbedded[plugin].input[2].value) {
				RcPluginEmbedded[plugin].output[0].value = 1000;
				sys_message("RC-Flow Alarm: ..");
			} else {
				RcPluginEmbedded[plugin].output[0].value = -1000;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_TIMER) {
			if (RcPluginEmbedded[plugin].input[1].value > 0) {
				RcPluginEmbedded[plugin].output[2].value = -1000;
			} else if (RcPluginEmbedded[plugin].input[0].value > 0 && RcPluginEmbedded[plugin].output[2].value < RcPluginEmbedded[plugin].input[2].value) {
				RcPluginEmbedded[plugin].output[2].value++;
			}
			if (RcPluginEmbedded[plugin].output[2].value >= RcPluginEmbedded[plugin].input[2].value) {
				RcPluginEmbedded[plugin].output[0].value = 1000;
			} else {
				RcPluginEmbedded[plugin].output[0].value = -1000;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_AND) {
			if (RcPluginEmbedded[plugin].input[0].value > 0 && RcPluginEmbedded[plugin].input[1].value > 0) {
				RcPluginEmbedded[plugin].output[0].value = 1000;
			} else {
				RcPluginEmbedded[plugin].output[0].value = -1000;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_OR) {
			if (RcPluginEmbedded[plugin].input[0].value > 0 || RcPluginEmbedded[plugin].input[1].value > 0) {
				RcPluginEmbedded[plugin].output[0].value = 1000;
			} else {
				RcPluginEmbedded[plugin].output[0].value = -1000;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_PULSE) {
			if (RcPluginEmbedded[plugin].input[0].value > 0 && RcPluginEmbedded[plugin].output[2].value <= 0) {
				RcPluginEmbedded[plugin].output[2].value = RcPluginEmbedded[plugin].input[1].value;
			} else if (RcPluginEmbedded[plugin].output[2].value > 1) {
				RcPluginEmbedded[plugin].output[2].value--;
				RcPluginEmbedded[plugin].output[0].value = 1000;
			} else if (RcPluginEmbedded[plugin].input[0].value <= 0) {
				RcPluginEmbedded[plugin].output[2].value = 0;
			}
			if (RcPluginEmbedded[plugin].output[2].value > 1) {
				RcPluginEmbedded[plugin].output[0].value = 1000;
			} else {
				RcPluginEmbedded[plugin].output[0].value = -1000;
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		} else if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_SINUS) {
			uint16_t speed = (RcPluginEmbedded[plugin].input[0].value + 1000) / 2 / 100;
			if (RcPluginEmbedded[plugin].output[2].value == 1) {
				if (RcPluginEmbedded[plugin].output[0].value > -1000 + speed) {
					RcPluginEmbedded[plugin].output[0].value -= speed;
				} else {
					RcPluginEmbedded[plugin].output[0].value = -1000;
					RcPluginEmbedded[plugin].output[2].value = 0;
				}
			} else {
				if (RcPluginEmbedded[plugin].output[0].value < 1000 - speed) {
					RcPluginEmbedded[plugin].output[0].value += speed;
				} else {
					RcPluginEmbedded[plugin].output[0].value = 1000;
					RcPluginEmbedded[plugin].output[2].value = 1;
				}
			}
			RcPluginEmbedded[plugin].output[0].value = rcflow_value_limit(RcPluginEmbedded[plugin].output[0].value, -1000, 1000);
			RcPluginEmbedded[plugin].output[1].value = RcPluginEmbedded[plugin].output[0].value * -1.0;
		}
	}
}

void rcflow_exit (void) {
	if (rcflow_fd != -1) {
		serial_close(rcflow_fd);
		rcflow_fd = -1;
	}
}

uint8_t rcflow_init (char *port, uint32_t baud) {
	char tmp_str[64];
	char tmp_str2[64];
	uint8_t plugin = 0;
	uint8_t input = 0;
	uint8_t output = 0;
	uint8_t link = 0;
	uint8_t n = 0;
	if (startup == 0) {
		startup = 1;
		strcpy(setup_name, "new.xml");
		for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
			RcPlugin[plugin].open = 1;
			RcPluginEmbedded[plugin].type = 255;
			RcPluginEmbedded[plugin].option = 0;
			for (input = 0; input < MAX_INPUTS; input++) {
				RcPlugin[plugin].input[input].name[0] = 0;
				RcPluginEmbedded[plugin].input[input].value = 0;
				RcPluginEmbedded[plugin].input[input].type = RCFLOW_INOUT_TYPE_BILINEAR;
				RcPluginEmbedded[plugin].input[input].option = 0;
			}
			for (output = 0; output < MAX_INPUTS; output++) {
				RcPlugin[plugin].output[output].name[0] = 0;
				RcPluginEmbedded[plugin].output[output].value = 0;
				RcPluginEmbedded[plugin].output[output].type = RCFLOW_INOUT_TYPE_BILINEAR;
				RcPluginEmbedded[plugin].output[output].option = 0;
			}
		}
		for (link = 0; link < MAX_LINKS; link++) {
			RcLinkEmbedded[link].from[0] = 255;
			RcLinkEmbedded[link].from[1] = 255;
			RcLinkEmbedded[link].to[0] = 255;
			RcLinkEmbedded[link].to[1] = 255;
		}
		plugin = 0;
		for (n = 0; n < 8; n++) {
			sprintf(RcPlugin[plugin].name, "in%i", n + 1);
			RcPlugin[plugin].type = RCFLOW_PLUGIN_INPUT;
			RcPlugin[plugin].x = -0.86;
//			RcPlugin[plugin].x = -0.66;
			RcPlugin[plugin].x = -1.3;
			RcPlugin[plugin].y = -0.76 + n * 0.2;
			strcpy(RcPlugin[plugin].input[0].name, "val");
			strcpy(RcPlugin[plugin].input[1].name, "trm");
			strcpy(RcPlugin[plugin].output[0].name, "out");
			strcpy(RcPlugin[plugin].output[1].name, "rev");
			if (n < 5) {
				sprintf(tmp_str, "adc;adc%i", n + 1);
				sprintf(tmp_str2, "%s;val", RcPlugin[plugin].name);
				rcflow_link(tmp_str, tmp_str2);
			}
			if (n < 4) {
				sprintf(tmp_str, "enc;enc%i", n + 1);
				sprintf(tmp_str2, "%s;trm", RcPlugin[plugin].name);
				rcflow_link(tmp_str, tmp_str2);
			}
			plugin++;
		}
		for (n = 0; n < 8; n++) {
			sprintf(RcPlugin[plugin].name, "out%i", n + 1);
			RcPlugin[plugin].type = RCFLOW_PLUGIN_OUTPUT;
			RcPlugin[plugin].x = 0.86;
//			RcPlugin[plugin].x = 0.66;
			RcPlugin[plugin].x = 1.3;
			RcPlugin[plugin].y = -0.76 + n * 0.2;
			strcpy(RcPlugin[plugin].input[0].name, "in");
			strcpy(RcPlugin[plugin].input[1].name, "trm");
			strcpy(RcPlugin[plugin].output[0].name, "val");
			strcpy(RcPlugin[plugin].output[1].name, "rev");
			if (n < 5) {
				sprintf(tmp_str, "%s;out", RcPlugin[plugin - 8].name);
				sprintf(tmp_str2, "%s;in", RcPlugin[plugin].name);
				rcflow_link(tmp_str, tmp_str2);
			}
			sprintf(tmp_str, "%s;val", RcPlugin[plugin].name);
			sprintf(tmp_str2, "ppm;ch%i", n + 1);
			rcflow_link(tmp_str, tmp_str2);
			plugin++;
		}
		strcpy(RcPlugin[0].title, "QUER");
		strcpy(RcPlugin[1].title, "HOEHE");
		strcpy(RcPlugin[2].title, "GAS");
		strcpy(RcPlugin[3].title, "SEITE");
		strcpy(RcPlugin[4].title, "KLAPPEN");
		strcpy(RcPlugin[8].title, "QUER_R");
		strcpy(RcPlugin[9].title, "HOEHE");
		strcpy(RcPlugin[10].title, "GAS");
		strcpy(RcPlugin[11].title, "SEITE");
		strcpy(RcPlugin[12].title, "QUER_L");

		rcflow_link("in1;rev", "out5;in");
		rcflow_link("in5;out", "out1;in");

		strcpy(RcPlugin[plugin].name, "adc");
		strcpy(RcPlugin[plugin].title, "ADC's");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_ADC;
		RcPlugin[plugin].x = -1.52;
//		RcPlugin[plugin].x = -1.3;
		RcPlugin[plugin].x = -2.0;
		RcPlugin[plugin].y = -0.46;
		strcpy(RcPlugin[plugin].output[0].name, "adc1");
		strcpy(RcPlugin[plugin].output[1].name, "adc2");
		strcpy(RcPlugin[plugin].output[2].name, "adc3");
		strcpy(RcPlugin[plugin].output[3].name, "adc4");
		strcpy(RcPlugin[plugin].output[4].name, "adc5");
		strcpy(RcPlugin[plugin].output[5].name, "adc6");
		strcpy(RcPlugin[plugin].output[6].name, "adc7");
		strcpy(RcPlugin[plugin].output[7].name, "adc8");
		strcpy(RcPlugin[plugin].output[8].name, "adc9");
		strcpy(RcPlugin[plugin].output[9].name, "adc10");
		strcpy(RcPlugin[plugin].output[10].name, "adc11");
		strcpy(RcPlugin[plugin].output[11].name, "adc12");
		plugin++;

		strcpy(RcPlugin[plugin].name, "enc");
		strcpy(RcPlugin[plugin].title, "Encoder");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_ENC;
		RcPlugin[plugin].x = -1.52;
//		RcPlugin[plugin].x = -1.3;
		RcPlugin[plugin].x = -2.0;
		RcPlugin[plugin].y = 0.14;
		strcpy(RcPlugin[plugin].output[0].name, "enc1");
		strcpy(RcPlugin[plugin].output[1].name, "enc2");
		strcpy(RcPlugin[plugin].output[2].name, "enc3");
		strcpy(RcPlugin[plugin].output[3].name, "enc4");
		plugin++;

		strcpy(RcPlugin[plugin].name, "sw");
		strcpy(RcPlugin[plugin].title, "Switches");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_SW;
		RcPlugin[plugin].x = -1.52;
//		RcPlugin[plugin].x = -1.3;
		RcPlugin[plugin].x = -2.0;
		RcPlugin[plugin].y = 0.74;
		strcpy(RcPlugin[plugin].output[0].name, "sw1");
		RcPlugin[plugin].output[0].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[0].value = -1000;
		strcpy(RcPlugin[plugin].output[1].name, "sw2");
		RcPlugin[plugin].output[1].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[1].value = -1000;
		strcpy(RcPlugin[plugin].output[2].name, "sw3");
		RcPlugin[plugin].output[2].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[2].value = -1000;
		strcpy(RcPlugin[plugin].output[3].name, "sw4");
		RcPlugin[plugin].output[3].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[3].value = -1000;
		strcpy(RcPlugin[plugin].output[4].name, "sw5");
		RcPlugin[plugin].output[4].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[4].value = -1000;
		strcpy(RcPlugin[plugin].output[5].name, "sw6");
		RcPlugin[plugin].output[5].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[5].value = -1000;
		strcpy(RcPlugin[plugin].output[6].name, "sw7");
		RcPlugin[plugin].output[6].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[6].value = -1000;
		strcpy(RcPlugin[plugin].output[7].name, "sw8");
		RcPlugin[plugin].output[7].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[7].value = -1000;
		strcpy(RcPlugin[plugin].output[8].name, "sw9");
		RcPlugin[plugin].output[8].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[8].value = -1000;
		strcpy(RcPlugin[plugin].output[9].name, "sw10");
		RcPlugin[plugin].output[9].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[9].value = -1000;
		strcpy(RcPlugin[plugin].output[10].name, "sw11");
		RcPlugin[plugin].output[10].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[10].value = -1000;
		strcpy(RcPlugin[plugin].output[11].name, "sw12");
		RcPlugin[plugin].output[11].type = RCFLOW_INOUT_TYPE_SWITCH;
		RcPlugin[plugin].output[11].value = -1000;

		plugin++;

		strcpy(RcPlugin[plugin].name, "ppm");
		strcpy(RcPlugin[plugin].title, "PPM-Out");
		RcPlugin[plugin].type = RCFLOW_PLUGIN_PPM;
		RcPlugin[plugin].x = 1.52;
//		RcPlugin[plugin].x = 1.3;
		RcPlugin[plugin].x = 2.0;
		RcPlugin[plugin].y = -0.46;
		strcpy(RcPlugin[plugin].input[0].name, "ch1");
		strcpy(RcPlugin[plugin].input[1].name, "ch2");
		strcpy(RcPlugin[plugin].input[2].name, "ch3");
		strcpy(RcPlugin[plugin].input[3].name, "ch4");
		strcpy(RcPlugin[plugin].input[4].name, "ch5");
		strcpy(RcPlugin[plugin].input[5].name, "ch6");
		strcpy(RcPlugin[plugin].input[6].name, "ch7");
		strcpy(RcPlugin[plugin].input[7].name, "ch8");
		strcpy(RcPlugin[plugin].input[8].name, "ch9");
		strcpy(RcPlugin[plugin].input[9].name, "ch10");
		strcpy(RcPlugin[plugin].input[10].name, "ch11");
		strcpy(RcPlugin[plugin].input[11].name, "ch12");
		plugin++;
		sprintf(tmp_str, "%s/models/rcflow.xml", get_datadirectory());
		rcflow_parseDoc(tmp_str);
		rcflow_undo_save();
	}

	SDL_Log("rcflow: init serial port...\n");
	rcflow_fd = serial_open(port, baud);

	return 0;
}

uint8_t rcflow_connection_status (void) {
	return 1;
}

void screen_rcflow (ESContext *esContext) {
	char tmp_str[64];
	uint8_t plugin = 0;
	uint8_t link = 0;
	uint8_t n = 0;
#ifdef ADC_TEST
	uint8_t output = 0;
	uint8_t start_flag = 0;
	int buffer_n = 0;
	while (read(rcflow_fd, buf, 1) > 0) {
		if (start_flag == 1 && buffer_n < 100) {
			if (buf[0] == '\n' || buf[0] == '\r') {
				if (buffer[0] == 'A' && buffer[1] == 'D' && buffer[2] == 'C' && buffer[3] == ':') {
					sscanf(buffer, "ADC:%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;", &adc[0], &adc[1], &adc[2], &adc[3], &adc[4], &adc[5], &adc[6], &adc[7], &adc[8], &adc[9], &adc[10], &adc[11]);
				} else if (buffer[0] == 'S' && buffer[1] == 'W' && buffer[2] == ':') {
					sscanf(buffer, "SW:%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;%i;", &sw[0], &sw[1], &sw[2], &sw[3], &sw[4], &sw[5], &sw[6], &sw[7], &sw[8], &sw[9], &sw[10], &sw[11]);
				} else if (buffer[0] == 'O' && buffer[1] == 'U' && buffer[2] == 'T' && buffer[3] == ':') {
					sscanf(buffer, "OUT:%i;%i;%i;%i;%i;%i;%i;%i;", &out[0], &out[1], &out[2], &out[3], &out[4], &out[5], &out[6], &out[7]);
					SDL_Log("rcflow: STM %i %i %i %i %i %i %i %i --\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
					SDL_Log("rcflow: LIN ");
					for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
						if (RcPluginEmbedded[plugin].type == RCFLOW_PLUGIN_PPM) {
							for (output = 0; output < 8; output++) {
								if (RcPluginEmbedded[plugin].input[output].option == 1) {
									SDL_Log("%i ", RcPluginEmbedded[plugin].input[output].value * -1);
								} else {
									SDL_Log("%i ", RcPluginEmbedded[plugin].input[output].value);
								}
							}
						}
					}
					SDL_Log("\n");
	//			} else if (buffer[0] == 'C' && buffer[1] == 'N' && buffer[2] == 'T' && buffer[3] == ':') {
	//				SDL_Log("rcflow: ################# %s ##\n", buffer);
				}
				start_flag = 0;
				buffer_n = 0;
			} else {
				buffer[buffer_n++] = buf[0];
				buffer[buffer_n] = 0;
			}
		} else {
			start_flag = 0;
			buffer_n = 0;
			if (buf[0] == 'A' || buf[0] == 'O' || buf[0] == 'C' || buf[0] == 'S') {
				start_flag = 1;
				buffer[buffer_n++] = buf[0];
				buffer[buffer_n] = 0;
			}
		}
	}
	//usleep(100000);
#endif


	// Calculating
	rcflow_convert_to_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_calc_Embedded();
	rcflow_convert_from_Embedded();

	redraw_flag = 1;

	if (virt_view == 1) {
		scale = 1.0;
		sprintf(tmp_str, "RC-Flow/Virtual-Controls (%s)", setup_name);
		draw_title(esContext, tmp_str);
		// Draw Virtual-Inputs
		n = 0;
		for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
			if (RcPlugin[plugin].name[0] != 0 && (RcPlugin[plugin].type == RCFLOW_PLUGIN_VADC || RcPlugin[plugin].type == RCFLOW_PLUGIN_VSW || RcPlugin[plugin].type == RCFLOW_PLUGIN_MULTIPLEX_IN || RcPlugin[plugin].vview == 1)) {
				rcflow_draw_vplugin(esContext, plugin, n++);
			}
		}
	} else if (virt_view == 2) {
		uint16_t plugin = 0;
		uint16_t plugin2 = 0;
		uint16_t input = 0;
		uint16_t output = 0;
		uint16_t link = 0;
		uint16_t type = 0;
		uint8_t linked = 0;
		uint8_t plist = 0;
		uint8_t tlist = 0;
		char tmp_str[128];
		char tmp_str2[128];
		uint16_t menu_y0 = 0;
		uint16_t menu_y1 = 0;
		uint16_t menu_y2 = 0;
		uint16_t menu_y3 = 0;
		uint16_t menu_y4 = 0;
		if (menu_set == 0) {
			draw_text_button(esContext, "m0", setup.view_mode, "Geber", FONT_PINK, -1.3, -0.7 + menu_y0++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set, 0);
			for (type = 0; type < RCFLOW_PLUGIN_LAST; type++) {
				tlist = 0;
				for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
					if (RcPlugin[plugin].name[0] != 0 && RcPlugin[plugin].type == type) {
						plist = 0;
						for (input = 0; input < MAX_INPUTS; input++) {
							if (RcPlugin[plugin].input[input].name[0] != 0) {
								linked = 0;
								if (RcPlugin[plugin].type != RCFLOW_PLUGIN_INPUT) {
									for (link = 0; link < MAX_LINKS; link++) {
										if (RcLink[link].from[0] != 0) {
											sprintf(tmp_str, "%s;%s", RcPlugin[plugin].name, RcPlugin[plugin].input[input].name);
											if (strcmp(RcLink[link].to, tmp_str) == 0) {
												linked = 1;
												break;
											}
										}
									}
								}
								if (linked == 0) {
									if (tlist == 0) {
										tlist = 1;
										if (menu_set_type == RcPlugin[plugin].type) {
											draw_text_button(esContext, plugintype_names[RcPlugin[plugin].type], setup.view_mode, plugintype_names[RcPlugin[plugin].type], FONT_PINK, -0.9, -0.7 + menu_y1++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_type, RcPlugin[plugin].type);
										} else {
											draw_text_button(esContext, plugintype_names[RcPlugin[plugin].type], setup.view_mode, plugintype_names[RcPlugin[plugin].type], FONT_GREEN, -0.9, -0.7 + menu_y1++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_type, RcPlugin[plugin].type);
										}
									}
									if (plist == 0) {
										plist = 1;
										if (menu_set_type == RcPlugin[plugin].type) {
											if (menu_set_plugin == plugin) {
												if (RcPlugin[plugin].title[0] == 0) {
													draw_text_button(esContext, RcPlugin[plugin].name, setup.view_mode, RcPlugin[plugin].name, FONT_PINK, -0.5, -0.7 + menu_y2++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_plugin, plugin);
												} else {
													draw_text_button(esContext, RcPlugin[plugin].name, setup.view_mode, RcPlugin[plugin].title, FONT_PINK, -0.5, -0.7 + menu_y2++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_plugin, plugin);
												}
											} else {
												if (RcPlugin[plugin].title[0] == 0) {
													draw_text_button(esContext, RcPlugin[plugin].name, setup.view_mode, RcPlugin[plugin].name, FONT_GREEN, -0.5, -0.7 + menu_y2++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_plugin, plugin);
												} else {
													draw_text_button(esContext, RcPlugin[plugin].name, setup.view_mode, RcPlugin[plugin].title, FONT_GREEN, -0.5, -0.7 + menu_y2++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_plugin, plugin);
												}
											}
										}
									}
									if (menu_set_type == RcPlugin[plugin].type && menu_set_plugin == plugin) {
										if (menu_set_input == input) {
											draw_text_button(esContext, RcPlugin[plugin].input[input].name, setup.view_mode, RcPlugin[plugin].input[input].name, FONT_PINK, -0.1, -0.7 + menu_y3++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_input, input);
										} else {
											draw_text_button(esContext, RcPlugin[plugin].input[input].name, setup.view_mode, RcPlugin[plugin].input[input].name, FONT_GREEN, -0.1, -0.7 + menu_y3++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_input, input);
										}
									}
								}
							}
						}
					}
				}
			}
			for (plugin2 = 0; plugin2 < MAX_PLUGINS; plugin2++) {
				if (RcPlugin[plugin2].name[0] != 0 && RcPlugin[plugin2].type == RCFLOW_PLUGIN_ADC) {
					for (output = 0; output < MAX_OUTPUTS; output++) {
						if (RcPlugin[plugin2].output[output].name[0] != 0) {
							linked = 0;
							for (link = 0; link < MAX_LINKS; link++) {
								if (RcLink[link].from[0] != 0 && menu_set_plugin != -1 && menu_set_input != -1) {
									sprintf(tmp_str, "%s;%s", RcPlugin[menu_set_plugin].name, RcPlugin[menu_set_plugin].input[menu_set_input].name);
									sprintf(tmp_str2, "%s;%s", RcPlugin[plugin2].name, RcPlugin[plugin2].output[output].name);
									if (strcmp(RcLink[link].to, tmp_str) == 0 && strcmp(RcLink[link].from, tmp_str2) == 0) {
										linked = 1;
										break;
									}
								}
							}
							sprintf(tmp_str, "%s_v", RcPlugin[plugin2].output[output].name);
							sprintf(tmp_str2, "%s = %i", RcPlugin[plugin2].output[output].name, RcPlugin[plugin2].output[0].value);
							if (linked == 1) {
								draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str2, FONT_PINK, 0.5, -0.7 + menu_y4++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_input, input);
							} else {
								draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str2, FONT_GREEN, 0.5, -0.7 + menu_y4++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_input, input);
							}
						}
					}
				}
			}
			menu_y4 = 0;
			for (plugin2 = 0; plugin2 < MAX_PLUGINS; plugin2++) {
				if (RcPlugin[plugin2].name[0] != 0 && RcPlugin[plugin2].type == RCFLOW_PLUGIN_SW) {
					for (output = 0; output < MAX_OUTPUTS; output++) {
						if (RcPlugin[plugin2].output[output].name[0] != 0) {
							linked = 0;
							for (link = 0; link < MAX_LINKS; link++) {
								if (RcLink[link].from[0] != 0 && menu_set_plugin != -1 && menu_set_input != -1) {
									sprintf(tmp_str, "%s;%s", RcPlugin[menu_set_plugin].name, RcPlugin[menu_set_plugin].input[menu_set_input].name);
									sprintf(tmp_str2, "%s;%s", RcPlugin[plugin2].name, RcPlugin[plugin2].output[output].name);
									if (strcmp(RcLink[link].to, tmp_str) == 0 && strcmp(RcLink[link].from, tmp_str2) == 0) {
										linked = 1;
										break;
									}
								}
							}
							sprintf(tmp_str, "%s_v", RcPlugin[plugin2].output[output].name);
							sprintf(tmp_str2, "%s = %i", RcPlugin[plugin2].output[output].name, RcPlugin[plugin2].output[0].value);
							if (linked == 1) {
								draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str2, FONT_PINK, 1.0, -0.7 + menu_y4++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_input, input);
							} else {
								draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str2, FONT_GREEN, 1.0, -0.7 + menu_y4++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_input, input);
							}
						}
					}
				}
			}
		} else {
			draw_text_button(esContext, "m0", setup.view_mode, "Geber", FONT_GREEN, -1.3, -0.7 + menu_y0++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set, 0);
		}
		if (menu_set == 1) {
			menu_y1 = 0;
			draw_text_button(esContext, "m1", setup.view_mode, "Reverse", FONT_PINK, -1.3, -0.7 + menu_y0++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set, 1);
			if (menu_set_reverse == 0) {
				draw_text_button(esContext, "r0", setup.view_mode, "Inputs", FONT_PINK, -0.9, -0.7 + menu_y1++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_reverse, 0);
				for (plugin2 = 0; plugin2 < MAX_PLUGINS; plugin2++) {
					if (RcPlugin[plugin2].name[0] != 0 && RcPlugin[plugin2].type == RCFLOW_PLUGIN_ADC) {
						for (output = 0; output < MAX_OUTPUTS; output++) {
							if (RcPlugin[plugin2].output[output].name[0] != 0) {
								sprintf(tmp_str, "o_%i_%i", plugin2, output);
								sprintf(tmp_str2, "%s = %i (%i)", RcPlugin[plugin2].output[output].name, RcPlugin[plugin2].output[output].value, RcPlugin[plugin2].output[output].option);
								draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str2, FONT_GREEN, 0.3, -0.7 + menu_y4++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_reverse_dir, output);
							}
						}
					}
				}
				menu_y4 = 0;
				for (plugin2 = 0; plugin2 < MAX_PLUGINS; plugin2++) {
					if (RcPlugin[plugin2].name[0] != 0 && RcPlugin[plugin2].type == RCFLOW_PLUGIN_SW) {
						for (output = 0; output < MAX_OUTPUTS; output++) {
							if (RcPlugin[plugin2].output[output].name[0] != 0) {
								sprintf(tmp_str, "o_%i_%i", plugin2, output);
								sprintf(tmp_str2, "%s = %i (%i)", RcPlugin[plugin2].output[output].name, RcPlugin[plugin2].output[output].value, RcPlugin[plugin2].output[output].option);
								draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str2, FONT_GREEN, 0.8, -0.7 + menu_y4++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_reverse_dir, output);
							}
						}
					}
				}
			} else {
				draw_text_button(esContext, "r0", setup.view_mode, "Inputs", FONT_GREEN, -0.9, -0.7 + menu_y1++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_reverse, 0);
			}
			if (menu_set_reverse == 1) {
				draw_text_button(esContext, "r1", setup.view_mode, "Outputs", FONT_PINK, -0.9, -0.7 + menu_y1++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_reverse, 1);
				for (plugin2 = 0; plugin2 < MAX_PLUGINS; plugin2++) {
					if (RcPlugin[plugin2].name[0] != 0 && RcPlugin[plugin2].type == RCFLOW_PLUGIN_PPM) {
						for (input = 0; input < MAX_INPUTS; input++) {
							if (RcPlugin[plugin2].input[input].name[0] != 0) {
								sprintf(tmp_str, "i_%i_%i", plugin2, input);
								sprintf(tmp_str2, "%s = %i (%i)", RcPlugin[plugin2].input[input].name, RcPlugin[plugin2].input[input].value, RcPlugin[plugin2].input[input].option);
								draw_text_button(esContext, tmp_str, setup.view_mode, tmp_str2, FONT_GREEN, 0.3, -0.7 + menu_y4++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_reverse_dir, input);
							}
						}
					}
				}
			} else {
				draw_text_button(esContext, "r1", setup.view_mode, "Outputs", FONT_GREEN, -0.9, -0.7 + menu_y1++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set_reverse, 1);
			}
		} else {
			draw_text_button(esContext, "m1", setup.view_mode, "Reverse", FONT_GREEN, -1.3, -0.7 + menu_y0++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set, 1);
		}
		if (menu_set == 2) {
			draw_text_button(esContext, "m2", setup.view_mode, "PPM", FONT_PINK, -1.3, -0.7 + menu_y0++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set, 2);
		} else {
			draw_text_button(esContext, "m2", setup.view_mode, "PPM", FONT_GREEN, -1.3, -0.7 + menu_y0++ * 0.08, 0.002, 0.06, ALIGN_LEFT, ALIGN_TOP, rcflow_menu_set, 2);
		}
	} else {
		// Drawing
		glDisable( GL_DEPTH_TEST );
#ifndef SIMPLE_DRAW
		draw_box_f3c2(esContext, -1.5, -1.0, 0.05, 1.5, 1.0, 0.05, 2, 2, 2, 255, 0x4d, 0x56, 0x67, 255);
#endif
		set_button("canvas", setup.view_mode, -1.5, -0.9, 1.5, 1.0, rcflow_canvas_move, (float)0, 2);
		// Draw Plugins
		for (plugin = 0; plugin < MAX_PLUGINS; plugin++) {
			if (RcPlugin[plugin].name[0] != 0) {
				rcflow_draw_plugin(esContext, plugin);
			}
		}
//SDL_Log("#1: \n", times(0));
		// Draw Links
		for (link = 0; link < MAX_LINKS; link++) {
			if (RcLink[link].from[0] != 0 && RcLink[link].to[0] != 0) {
				rcflow_draw_link(esContext, link);
			}
		}
#ifndef SIMPLE_DRAW
		draw_box_f3c2(esContext, -1.5, 0.8, 0.001, 1.5, 1.0, 0.001, 0, 0, 0, 0, 0, 0, 0, 255);
#endif
		draw_box_f3(esContext, -1.3 - 0.05, 0.9 - 0.05, 0.001, -1.3 + 0.05, 0.9 + 0.05, 0.001, 255, 255, 255, 200);
		draw_box_f3(esContext, -1.3 - 0.007 - canvas_x / 40.0, 0.9 - 0.007 - canvas_y / 40.0, 0.001, -1.3 + 0.007 - canvas_x / 40.0, 0.9 + 0.007 - canvas_y / 40.0, 0.001, 0, 0, 0, 255);
		set_button("canvas_center", setup.view_mode, -1.3 - 0.05, 0.9 - 0.05, -1.3 + 0.05, 0.9 + 0.05, rcflow_canvas_center, (float)0, 0);
		sprintf(tmp_str, "RC-Flow/Setup (%s)", setup_name);
		draw_title(esContext, tmp_str);
		if (link_select_from[0] != 0 && link_select_to[0] != 0) {
			draw_text_button(esContext, "add_link", setup.view_mode, "[LINK]", FONT_GREEN, 0.0, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_link_add, 0.0);
		} else if (link_select_from[0] != 0 || link_select_to[0] != 0) {
			draw_text_button(esContext, "add_link", setup.view_mode, "[LINK]", FONT_PINK, 0.0, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_link_add, 0.0);
		} else {
			draw_text_button(esContext, "add_link", setup.view_mode, "[LINK]", FONT_TRANS, 0.0, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_link_add, 0.0);
		}
		if (del_plugin == 0) {
			draw_text_button(esContext, "plugin_del", setup.view_mode, "[DEL]", FONT_WHITE, -0.3, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_del, 0.0);
		} else {
			draw_text_button(esContext, "plugin_del", setup.view_mode, "[DEL]", FONT_GREEN, -0.3, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_del, 0.0);
		}
		draw_text_button(esContext, "plugin_add", setup.view_mode, "[ADD]", FONT_GREEN, 0.3, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_select, 0.0);
	}

	draw_text_button(esContext, "virtview", setup.view_mode, "[VIRT]", FONT_GREEN, -1.1, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_virt_view, 0.0);
	draw_text_button(esContext, "load", setup.view_mode, "[LOAD]", FONT_GREEN, 0.6, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_load, 0.0);
	if (unsave == 0) {
		draw_text_button(esContext, "save", setup.view_mode, "[SAVE]", FONT_GREEN, 0.85, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_save, 0.0);
	} else {
		draw_text_button(esContext, "save", setup.view_mode, "[SAVE]", FONT_PINK, 0.85, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_save, 0.0);
	}
	if (lock == 0) {
		lock_timer = 0;
		draw_text_button(esContext, "lock", setup.view_mode, "[LOCK]", FONT_WHITE, 1.1, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_lock, 0.0);
	} else if (lock_timer > 0) {
		lock_timer--;
		draw_text_button(esContext, "lock", setup.view_mode, "[LOCK]", FONT_PINK, 1.1, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_lock, 0.0);
	} else {
		draw_text_button(esContext, "lock", setup.view_mode, "[LOCK]", FONT_GREEN, 1.1, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_lock, 0.0);
	}
	if (undo_timer > 0) {
		draw_text_button(esContext, "undo", setup.view_mode, "[UNDO]", FONT_PINK, -0.85, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_undo, 0.0);
	} else if (undo < MAX_UNDO) {
		draw_text_button(esContext, "undo", setup.view_mode, "[UNDO]", FONT_GREEN, -0.85, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_undo, 0.0);
	} else {
		draw_text_button(esContext, "undo", setup.view_mode, "[UNDO]", FONT_TRANS, -0.85, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_undo, 0.0);
	}
	if (undo > 0) {
		draw_text_button(esContext, "redo", setup.view_mode, "[REDO]", FONT_GREEN, -0.6, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_redo, 0.0);
	} else {
		draw_text_button(esContext, "redo", setup.view_mode, "[REDO]", FONT_TRANS, -0.6, 0.9, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_redo, 0.0);
	}
	if (unsave == 1) {
		unsave = 2;
		undo_timer = 55;
	}
	if (undo_timer == 1) {
		rcflow_undo_save();
		undo_timer--;
	} else if (undo_timer > 0) {
		undo_timer--;
	}
	if (select_plugin == 1) {
		reset_buttons();
		draw_box_f3(esContext, -1.5, -1.0, 0.002, 1.5, 1.0, 0.002, 0, 0, 0, 200);
		draw_title(esContext, "Plugins");
		plugin = 0;
		draw_text_button(esContext, "add_vadc", setup.view_mode, "Virtual-ADC", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_VADC);
		plugin++;
		draw_text_button(esContext, "add_vsw", setup.view_mode, "Virtual-Switch", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_VSW);
		plugin++;
		draw_text_button(esContext, "add_input", setup.view_mode, "Input", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_INPUT);
		plugin++;
		draw_text_button(esContext, "add_output", setup.view_mode, "Output", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_OUTPUT);
		plugin++;
		draw_text_button(esContext, "add_mixer", setup.view_mode, "Mixer", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_MIXER);
		plugin++;
		draw_text_button(esContext, "add_atv", setup.view_mode, "ATV", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_ATV);
		plugin++;
		draw_text_button(esContext, "add_limit", setup.view_mode, "Limiter", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_LIMIT);
		plugin++;
		draw_text_button(esContext, "add_delay", setup.view_mode, "Delay (Slowmotion)", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_DELAY);
		plugin++;
		draw_text_button(esContext, "add_curve", setup.view_mode, "Curve (7-Point)", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_CURVE);
		plugin++;
		draw_text_button(esContext, "add_butter", setup.view_mode, "BUTTERFLY", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_BUTTERFLY);
		plugin++;
		draw_text_button(esContext, "add_mpxi", setup.view_mode, "MPX-In", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_MULTIPLEX_IN);
		plugin++;
		draw_text_button(esContext, "add_mpxo", setup.view_mode, "MPX-Out", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_MULTIPLEX_OUT);
		plugin++;
		draw_text_button(esContext, "add_multiv", setup.view_mode, "Multi-Value", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_MULTIVALUE);
		plugin++;
		draw_text_button(esContext, "add_tcl", setup.view_mode, "TCL (Script)", FONT_GREEN, -0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_TCL);
		plugin++;

		plugin = 0;
		draw_text_button(esContext, "add_expo", setup.view_mode, "Expo", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_EXPO);
		plugin++;
		draw_text_button(esContext, "add_heli120", setup.view_mode, "Heli-120 (Swashplate)", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_H120);
		plugin++;
		draw_text_button(esContext, "add_switch", setup.view_mode, "Switch (Changeover)", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_SWITCH);
		plugin++;
		draw_text_button(esContext, "add_range", setup.view_mode, "Range (Linear to Switch)", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_RANGESW);
		plugin++;
		draw_text_button(esContext, "add_alarm", setup.view_mode, "Alarm", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_ALARM);
		plugin++;
		draw_text_button(esContext, "add_timer", setup.view_mode, "Timer (Set / Reset)", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_TIMER);
		plugin++;
		draw_text_button(esContext, "add_and", setup.view_mode, "LOGIC_AND", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_AND);
		plugin++;
		draw_text_button(esContext, "add_or", setup.view_mode, "LOGIC_OR", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_OR);
		plugin++;
		draw_text_button(esContext, "add_pulse", setup.view_mode, "Pulse (Short Pulse)", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_PULSE);
		plugin++;
		draw_text_button(esContext, "add_sinus", setup.view_mode, "Sinus (Auto-Input)", FONT_GREEN, 0.5, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)RCFLOW_PLUGIN_SINUS);
		plugin++;

		plugin++;
		plugin++;
		draw_text_button(esContext, "back", setup.view_mode, "[BACK]", FONT_GREEN, 0.0, -0.8 + plugin * 0.1, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_plugin_add, (float)255);
		plugin++;
	}

	draw_text_button(esContext, "update", setup.view_mode, "[UPDATE]", FONT_GREEN, 0.0, 0.8, 0.002, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_update, 0.0);

	static int cursor_pos = -1;

	if (rcflow_sript_view_flag != -1) {
		draw_box_f3(esContext, -1.0, -0.7, 0.001, 1.0, 0.7, 0.001, 255, 255, 255, 255);

		int n = 0;
		int n2 = 0;
		float txt_x = -1.0;
		float txt_y = -0.7;
		char new_char = 0;
		char tmp_str[2];
		char new_text[2048];

		if (keyboard_key[0] != 0) {
			if (strcmp(keyboard_key, "return") == 0) {
				new_char = '\n';
			} else if (strcmp(keyboard_key, "backspace") == 0) {
				new_char = -1;
			} else if (strcmp(keyboard_key, "delete") == 0) {
				new_char = -2;
			} else if (strcmp(keyboard_key, "up") == 0) {
				new_char = -3;
			} else if (strcmp(keyboard_key, "down") == 0) {
				new_char = -4;
			} else if (strcmp(keyboard_key, "end") == 0) {
				cursor_pos = strlen(rcflow_sript_view_text);
			} else if (strcmp(keyboard_key, "home") == 0) {
				cursor_pos = 0;
			} else if (strcmp(keyboard_key, "escape") == 0) {
				rcflow_sript_view_flag = -1;
			} else if (strcmp(keyboard_key, "left") == 0) {
				if (cursor_pos > 0) {
					cursor_pos--;
				}
			} else if (strcmp(keyboard_key, "right") == 0) {
				if (cursor_pos < strlen(rcflow_sript_view_text)) {
					cursor_pos++;
				}
			} else if (strcmp(keyboard_key, "space") == 0) {
				new_char = ' ';
			} else if (keyboard_key[1] == 0) {
				new_char = keyboard_key[0];
			} else {
				new_char = 0;
			}
			keyboard_key[0] = 0;
		}

		if (cursor_pos == -1 || cursor_pos > strlen(rcflow_sript_view_text)) {
			cursor_pos = strlen(rcflow_sript_view_text) - 1;
		}

		int last_ret = 0;
		int last_ret2 = 0;
		int last_pos = 0;
		tmp_str[1] = 0;
		for (n = 0; n < strlen(rcflow_sript_view_text) + 1; n++) {
			if (n2 == cursor_pos) {
				if (new_char == -1) {
					n2--;
					cursor_pos--;
					new_char = 0;
				} else if (new_char == -2) {
					n++;
					new_char = 0;
				} else if (new_char == -3) {
					cursor_pos = last_ret2 + last_pos + 1;
					new_char = 0;
				} else if (new_char == -4) {
				} else if (new_char != 0) {
					txt_x += 0.05;
					new_text[n2++] = new_char;
					cursor_pos++;
					new_char = 0;
				}
				draw_text_f3(esContext, txt_x + 0.05, txt_y + 0.005, 0.005, 0.05, 0.05, FONT_GREEN, "_");
			}
			new_text[n2] = rcflow_sript_view_text[n];
			new_text[n2 + 1] = 0;

			if (rcflow_sript_view_text[n] == '\n') {
				if (n2 > cursor_pos && new_char == -4) {
					cursor_pos = n2 + last_pos + 1;
					new_char = 0;
				}
				txt_x = -1.0;
				txt_y += 0.05;
				last_ret2 = last_ret;
				last_ret = n2;
				last_pos = 0;
			} else {
				if (n2 < cursor_pos) {
					last_pos++;
				}
				txt_x += 0.05;
				tmp_str[0] = new_text[n2];
				draw_text_f3(esContext, txt_x, txt_y, 0.005, 0.05, 0.05, FONT_GREEN, tmp_str);
			}
			n2++;
		}
		new_text[n2] = 0;
		strcpy(rcflow_sript_view_text, new_text);

		draw_text_button(esContext, "script_save", setup.view_mode, "[SAVE]", FONT_GREEN, 0.0 - 0.15, 0.6, 0.005, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_sript_view_save, (float)-1);
		draw_text_button(esContext, "script_close", setup.view_mode, "[CANCEL]", FONT_GREEN, 0.0 + 0.15, 0.6, 0.005, 0.06, ALIGN_CENTER, ALIGN_TOP, rcflow_sript_view, (float)-1);
	} else {
		cursor_pos = -1;
	}

	if (keyboard_key[0] != 0) {
		SDL_Log("rcflow: keyboard_key(shift) = %s (%i)\n", keyboard_key, keyboard_shift);
	}


	screen_keyboard(esContext);
	screen_filesystem(esContext);

	glEnable( GL_DEPTH_TEST );


//	rcflow_tcl_run("puts $ModelData[ModelActive](name)");


//SDL_Log("%i %i %i\n", (int)sizeof(RcPluginEmbedded), (int)sizeof(RcLinkEmbedded), (int)sizeof(RcPluginEmbedded) + (int)sizeof(RcLinkEmbedded));


}


