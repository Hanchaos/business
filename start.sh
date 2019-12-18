#!/bin/sh
WORK_DIR=`dirname $0`
if [ "$WORK_DIR" = "." ];then
    WORK_DIR=$PWD
fi

PNAME=xbusiness

usage() {
cat <<EOF
Usage:
  start with daemon:
    \$ $0
    \$ $0 daemon
  start without daemon:
    \$ PROGRAM_NO_DAEMON=true $0
    \$ $0 nodaemon
EOF
}
 
prepare() {
    export LD_LIBRARY_PATH=$WORK_DIR/lib:$LD_LIBRARY_PATH
    export LD_LIBRARY_PATH=${WORK_DIR}/${PNAME}:$LD_LIBRARY_PATH
}
 
run_as_no_daemon() {
    cd $WORK_DIR
    cd ./bin && ./${PNAME}
}
 
run_as_daemon() {
    [ -d $WORK_DIR/log ] || mkdir $WORK_DIR/log
    cd $WORK_DIR
    cd ./bin && nohup ./${PNAME} >> $WORK_DIR/log/stdout.log 2>>$WORK_DIR/log/stderr.log &
}
 
prepare
 
case $1 in
    "nodaemon")
        run_as_no_daemon
    ;;
    "daemon")
        run_as_daemon
    ;;
    "")
        if [ -z "$PROGRAM_NO_DAEMON" ];then
            run_as_daemon
        else
            run_as_no_daemon
        fi
    ;;
    *)
        usage
    ;;
esac

