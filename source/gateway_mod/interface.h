#pragma once
#include <string>
#include "type.pb.h"
#include "context.h"
#include "pool.h"

#include "data_structures.h"

std::string pb2xml(std::unique_ptr<google::protobuf::Message>& response);
std::string pb2txt(std::unique_ptr<google::protobuf::Message>& response);
//std::string pb2json(std::unique_ptr<google::protobuf::Message>& response, int depth = 0);

void render(webservice::RequestData& request, webservice::ResponseData& response, Context& context);


void render_status(webservice::RequestData& request, webservice::ResponseData& response, Context& context, Pool& pool);

