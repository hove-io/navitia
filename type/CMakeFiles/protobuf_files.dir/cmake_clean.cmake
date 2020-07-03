file(REMOVE_RECURSE
  "../source/jormungandr/tests/chaos_pb2.py"
  "../source/jormungandr/tests/gtfs_realtime_pb2.py"
  "../source/jormungandr/tests/kirin_pb2.py"
  "../source/monitor/monitor_kraken/request_pb2.py"
  "../source/monitor/monitor_kraken/response_pb2.py"
  "../source/monitor/monitor_kraken/type_pb2.py"
  "../source/navitiacommon/navitiacommon/request_pb2.py"
  "../source/navitiacommon/navitiacommon/response_pb2.py"
  "../source/navitiacommon/navitiacommon/stat_pb2.py"
  "../source/navitiacommon/navitiacommon/task_pb2.py"
  "../source/navitiacommon/navitiacommon/type_pb2.py"
  "CMakeFiles/protobuf_files"
  "chaos.pb.cc"
  "chaos.pb.h"
  "gtfs-realtime.pb.cc"
  "gtfs-realtime.pb.h"
  "kirin.pb.cc"
  "kirin.pb.h"
  "request.pb.cc"
  "request.pb.h"
  "response.pb.cc"
  "response.pb.h"
  "task.pb.cc"
  "task.pb.h"
  "type.pb.cc"
  "type.pb.h"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/protobuf_files.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
