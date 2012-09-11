#pragma once
#include "type/type.pb.h"
#include <memory>

/// Crée une version XML d'un message protocol buffer
std::string pb2xml(const google::protobuf::Message* response);

/// Crée une version JSON d'un message protocol buffer
std::string pb2json(const google::protobuf::Message* response, int depth = 0);

/// Construit le message protocol buffer correspondant au nom de l'api
std::unique_ptr<pbnavitia::Response> create_pb();
