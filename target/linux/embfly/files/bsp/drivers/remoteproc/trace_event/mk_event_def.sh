#!/bin/bash

Kconfig_file=event.Kconfig
head_file=./include/trace_event_def.h
src_file=trace_event_def.c

cat event_list.txt | awk ' /EV_*/ { printf "config EVENTS_TRACE_%s\n\tbool ", $1; \
		for (i=3;i<NF;i++) { printf "%s ", $i } \
		printf "%s\n", $NF; \
		printf "\tdefault n\n"
		printf "config EVENTS_TRACE_%s_DEFAULT\n\tbool ", $1;
		printf "\"Enable %s in default\"\n", $1; \
		printf "\tdepends on EVENTS_TRACE_%s\n", $1; \
		printf "\tdefault n\n\n"}' > $Kconfig_file

echo "/* this file is generate by ./mk_event_def.sh, should not be modify directly */" > $head_file
echo "#ifndef _TRACE_EVENT_DEF_H_" >> $head_file
echo "#define _TRACE_EVENT_DEF_H_" >> $head_file
echo "" >> $head_file
echo "typedef enum {" >> $head_file
cat event_list.txt | awk ' /EV_*/ { printf "\t%s,\n", $1 }' >> $head_file
echo -e "\tEV_NUM_SUBSYS" >> $head_file
echo "} event_subsys;" >> $head_file
echo "" >> $head_file
cat event_list.txt | awk ' /EV_*/ { printf "%s_STRING %s\n", $1, $2 }'| \
		awk '{ printf "#define %-30s %s\n",$1,$2 }' >> $head_file
echo "" >> $head_file
echo "#endif" >> $head_file

echo "/* this file is generate by ./mk_event_def.sh, should not be modify directly */" > $src_file
#echo "#include <stdio.h>" >> $src_file
echo '#include "include/trace_event.h"' >> $src_file
echo "" >> $src_file
echo "#ifdef TRACE_EVENT_PARSER_ON_NATIVE_PLATFORM" >> $src_file
echo "struct subsys_entry g_subsys_class[EV_NUM_SUBSYS] = {" >> $src_file
# [n] = { .name = n##_STRING, .enable = 1 }
cat event_list.txt | \
		awk ' /EV_*/ { printf "\t[%s] = {\n", $1; \
				printf "\t\t.name = %s_STRING,\n", $1; \
				printf "\t\t.enable = EV_MACRO_IS_ENABLED(CONFIG_EVENTS_TRACE_%s_DEFAULT),\n", $1; \
				printf "\t\t.buildin = EV_MACRO_IS_ENABLED(CONFIG_EVENTS_TRACE_%s)\n", $1; \
				printf "\t},\n"; }' >> $src_file
echo "};" >> $src_file
echo "#endif" >> $src_file
