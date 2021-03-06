#!/bin/sh
#
#

EXTRA_HEADERS="mavlink/GCS_MAVLink/include/mavlink/v1.0/autoquad/mavlink_msg_aq_esc_telemetry.h"

echo ""
echo "#include <all.h>"
for EXTRA_HEADER in $EXTRA_HEADERS
do
	echo "#include <$EXTRA_HEADER>"
done
echo "#include <mavlink/GCS_MAVLink/include/mavlink/v1.0/autoquad/mavlink_msg_aq_telemetry_f.h>"
echo "#include <mavlink/GCS_MAVLink/include/mavlink/v1.0/pixhawk/mavlink_msg_set_netid.h>"
echo ""
echo "uint8_t mavlink_target_rewrite (mavlink_message_t* msg) {"
echo "	uint8_t modelid = 0;"
echo "	switch (msg->msgid) {"
echo "		case 0: {"
echo "			break;"
echo "		}"
grep -R "packet.target_system\>" mavlink/GCS_MAVLink/include/mavlink/v1.0/ | grep /mavlink_msg_ | cut -d":" -f1 | sort -u | while read F
do
	MSG_ID="`grep "MAVLINK_MSG_ID_" "$F" | head -n1 | sed "s|^#define ||g" | cut -d" " -f1`"
	NAME="`echo $MSG_ID | cut -d"_" -f4- | tr "A-Z" "a-z"`"
	echo "#ifdef $MSG_ID"
	echo "		case $MSG_ID: {"
	echo "			mavlink_${NAME}_t packet;"
	echo "			mavlink_msg_${NAME}_decode(msg, &packet);"
	echo "			modelid = packet.target_system - 1;"
	echo "			packet.target_system = ModelData[modelid].mavlink_org_sysid;"
	echo "			mavlink_msg_${NAME}_encode(msg->sysid, msg->compid, msg, &packet);"
	echo "			return modelid;"
	echo "			break;"
	echo "		}"
	echo "#endif"
done
echo "		default: {"
echo "			SDL_Log(\"mavlink_rewrite: ## BROADCAST MSG_ID == %i ##\\\n\", msg->msgid);"
echo "			break;"
echo "		}"
echo "	}"
echo "	return 255;"
echo "}"
echo ""
echo "uint8_t mavlink_source_rewrite (uint8_t modelid, mavlink_message_t* msg) {"
echo "	switch (msg->msgid) {"
grep -R "MAVLINK_MSG_.*_CRC\>" mavlink/GCS_MAVLink/include/mavlink/v1.0/common/ mavlink/GCS_MAVLink/include/mavlink/v1.0/ardupilotmega/ $EXTRA_HEADERS | grep /mavlink_msg_ | cut -d":" -f1 | sort -u | while read F
do
	MSG_ID="`grep "MAVLINK_MSG_ID_" "$F" | head -n1 | sed "s|^#define ||g" | cut -d" " -f1`"
	NAME="`echo $MSG_ID | cut -d"_" -f4- | tr "A-Z" "a-z"`"
	echo "#ifdef $MSG_ID"
	echo "		case $MSG_ID: {"
	echo "			mavlink_${NAME}_t packet;"
	echo "			mavlink_msg_${NAME}_decode(msg, &packet);"
	echo "			ModelData[modelid].mavlink_org_sysid = msg->sysid;"
	echo "			msg->sysid = modelid + 1;"
	echo "			mavlink_msg_${NAME}_encode(msg->sysid, msg->compid, msg, &packet);"
	echo "			return ModelData[modelid].mavlink_org_sysid;"
	echo "			break;"
	echo "		}"
	echo "#endif"
done
echo "		default: {"
echo "			SDL_Log(\"mavlink_rewrite: ## UNSUPPORTED MSG_ID == %i (mavlink/get_case_by_file.sh %i) ##\\\n\", msg->msgid, msg->msgid);"
echo "			break;"
echo "		}"
echo "	}"
echo "	return 255;"
echo "}"
echo ""
echo ""
echo ""



