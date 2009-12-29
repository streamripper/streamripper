rm -rf rip1
rm streamripper-debug-log.txt

STREAMRIPPER=~/build/streamripper/streamripper

URL=http://localhost:8000/ices.ogg
URL=http://localhost:8000/ices
URL=http://localhost:8000/stream

#PARSE_RULES="-w parse_rules.txt"
#PARSE_RULES="-w rules1.txt"
PARSE_RULES=

##CODESET="--codeset-metadata=iso-8859-15"
##CODESET="--codeset-metadata=CP1251"
CODESET="--codeset-id3=iso-8859-1"
CODESET=

OVERWRITE="-o version"
OVERWRITE=

TIMER="-l 14"
TIMER=

XS2="--xs2"
XS2=

SHOWFILE="-a testme.ogg"
SHOWFILE="-a testme.mp3"
SHOWFILE="-a testme"
SHOWFILE=

$STREAMRIPPER $URL $TIMER $SHOWFILE --debug -d rip1 -R 0 -r 8080 $XS2 $PARSE_RULES $CODESET $OVERWRITE -t
