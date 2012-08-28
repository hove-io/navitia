#pragma once

//forward declare
namespace webservice{
    struct RequestData;
    struct ResponseData;
}
namespace navitia{ namespace gateway{
    struct Context;
    struct Pool;
}}

namespace pbnavitia{
    class Response;
}

namespace navitia{

void render(webservice::RequestData& request, webservice::ResponseData& response, navitia::gateway::Context& context);
void render(webservice::RequestData& request, webservice::ResponseData& response, pbnavitia::Response& pb);

void render_status(webservice::RequestData& request, webservice::ResponseData& response, navitia::gateway::Context& context, navitia::gateway::Pool pool);
}
