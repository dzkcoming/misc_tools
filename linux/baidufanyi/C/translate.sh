#!/bin/bash

CUR_PATH=`dirname $0`
#echo $CUR_PATH
for i in {1..100000}
do
	echo -n "input a word : "
	read para
	#echo $para
	$CUR_PATH/app "$para" > $CUR_PATH/tmp
	#string=`cat tmp | grep dst | cut -d , -f 4 | cut -d '"' -f 4`
	string=`cat $CUR_PATH/tmp | grep dst | cut -d '"' -f 18`

	echo -n "   result    : "
	printf $string
	echo ""
	rm tmp

done
exit
