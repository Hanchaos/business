{{- $my := .services.serviceXbusiness -}}
[server]
brokers={{ joinNodes .instances.kafka }}
group=xbusiness
topics=tp_business_service
commit_num=10
{{- with .services.serviceBusiness }}
max_cache_size={{ .max_cache_size }}
{{- end }}
enable_conf_dump=false

[common]
io_thread_num = 8
httpretrytime=3
worker_thread_num=20
max_task_num=5000

[s3proxy]
{{- with .services.schedulerPicserver }}
ipport={{ .host}}:{{ .port }}
{{- end }}
path=/downloadByKey
gen_url_path=/image/url


[trace_space]
{{- with .services.serviceXtrace }}
ipport={{ .host }}:{{ .port }}
{{- end }}

[redis]
sentinel_name={{ .instances.redis.mastername }}
addrs={{ joinNodes .instances.redis.sentinels }}
passwd={{ .instances.redis.password }}
io_num=2
sub_key=__keyevent@0__:*
config_key=notify-keyspace-events
config_value=E$
db_index=0
pic_index=9
refresh_interval=3600000

[kafka]
brokers={{ joinNodes .instances.kafka }}


[warehouse]
set_name=test
version=1.0.0
house_path=../pattern/test.zip


[business_capture]
topic=busi_pass_tp_device_event
[common_capture]
topic=busi_pass_tp_device_capture
[passenger_flow]
topic=busi_pass_tp_device_event
[screenshot]
topic=tp_busiscreenshot_query


[mongo_business]
mongo_url={{ makeMongoUrl .instances.mongodb .databases.mongodb.businessJob }}
mongo_dbname={{ .databases.mongodb.businessJob.name }}
