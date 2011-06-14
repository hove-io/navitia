#include <string>
#include "type.pb.h"
#include "context.h"

#include "data_structures.h"

std::string pb2xml(std::unique_ptr<google::protobuf::Message>& response);
std::string pb2txt(std::unique_ptr<google::protobuf::Message>& response);

void render(webservice::RequestData& request, webservice::ResponseData& response, Context& context);
