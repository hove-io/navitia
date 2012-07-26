#pragma once
#include "context.h"
#include "pool.h"
#include "WS_commons/data_structures.h"


namespace navitia{ namespace gateway{

void render(webservice::RequestData& request, webservice::ResponseData& response, Context& context);
void render_status(webservice::RequestData& request, webservice::ResponseData& response, Context& context, Pool pool);

}}
