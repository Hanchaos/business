[logger_xbusiness]
SinkList=xbusinesslog|console #When config multiple sink, use '|' as delimiter to split sinks. mandatory.
Mode=asyn # "asyn", or "syn";  optional , defaule value is asyn
LogLevel=trace  #"trace", "debug", "info",  "warning", "error" or "critical";optional, default value is error.
#log formate configuration. please refer to https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
#LogFormat=#optional , default value is %+, the log output format will be like this: "[2014-31-10 23:46:59.678] [mylogger] [info] Some message"

#Asyn.QueueSize =8#asyn queue max size of log items. must be power of 2.  optional, default value is 8 * 1024
#Asyn.OverflowPolicy =#"block_retry" or "discard_log_msg". optional, default value is block_retry
Asyn.FlushIntervalMs=100#asyn flush interval of milliseconds. optional, default value is zero that means no flush.

#sink configuration
[sink_console] #sink_xxx, xxx is sink name. As this example, 'console' is the sink name.
SinkType=stdout_color #"stdout‘, “stdout_color”, “stderr”, “stderr_color”, “basic” , or “rotate”;mandatory
SinkLevel=trace-critical #lowlevel-highlevel ; optional, default value: info-critical
#FilenNme =#Filename must be configured when SinkType is basic or rotate. It can be relative path or absolute path.

#Rotate.MaxFileSize =#rotate log file when current log file size exceed this value. optional, default value is 50 * 1024 * 1024 byte
#Rotate.MaxFileNum =#retain maximum number of log files. optional, default value is 10

[sink_xbusinesslog]
SinkType=rotate
SinkLevel=trace-critical
FilenNme=xbusiness.log #FileName must be configured when SinkType is basic or rotate

Rotate.MaxFileSize=104857600
Rotate.MaxFileNum=10

