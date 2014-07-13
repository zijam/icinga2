#!/usr/bin/env bash
#
# send service notifications via short message (SMS)
# 
# uses smstools http://smstools3.kekekasvi.com/
# the icinga command user must be member in the group smstools uses

SMSTOOLS_GROUP=smstools
SMSTOOLS_OUTGOING_DIR=/var/spool/sms/outgoing

TMPFILE=`mktemp /tmp/smsd_XXXXXX`

echo "To: $USERPAGER" >$TMPFILE
echo "Alphabet: ISO" >>$TMPFILE
echo "" >>$TMPFILE

DURATION_MIN=$[$DURATION_SEC/60]

cat <<TEMPLATE | iconv -t ISO-8859-15 >>$TMPFILE
$NOTIFICATIONTYPE - $HOSTDISPLAYNAME - $SERVICEDISPLAYNAME is $SERVICESTATE for ${DURATION_MIN}m
($SERVICEOUTPUT)
TEMPLATE

chmod 660 $TMPFILE
chgrp $SMSTOOLS_GROUP $TMPFILE

MAXRENAME=10
while ! mv -n $TMPFILE $SMSTOOLS_OUTGOING_DIR; do
	# target file seems to already exist

	if [ $MAXRENAME -le 0 ]; then
		echo "can't write to target directory"
		exit 1
	fi

	NEWTMP=`mktemp /tmp/smsd_XXXXXX`
	mv -f $TMPFILE $NEWTMP
	TMPFILE=$NEWTMP

	MAXRENAME=$[$MAXRENAME-1]
done

