#!/bin/sh

FILE=$1
if [ -z "$FILE" ]; then
  FILE=armlist.template
fi

tmp=`cat $FILE | grep -v "^#" | wc -`
ntotal=`echo $tmp | cut -d' ' -f 1`

rm -f err1.txt err2.txt
touch err1.txt

while [ true ]; do
  grep -n error: armtest.log | grep -v "[#]warning" >err2.txt
  diff err1.txt err2.txt | sed -e "s/^[>] [0-9]*://g"
  mv err2.txt err1.txt

  wc -l armtest.log
  tmp=`grep Configuring armtest.log | wc -`
  nsofar=`echo $tmp | cut -d' ' -f 1`
  echo $nsofar of $ntotal Configurations
  sleep 180
done

