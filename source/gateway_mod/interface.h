#include <string>
#include "type.pb.h"
#include "context.h"




std::string pb2xml(std::unique_ptr<google::protobuf::Message>& response);
