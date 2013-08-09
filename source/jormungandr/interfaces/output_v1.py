# coding=utf8
from conf import base_url
from instance_manager import NavitiaManager, DeadSocketException, RegionNotFound
from renderers import render
import request_pb2, type_pb2,response_pb2
from uri import collections_to_resource_type, resource_type_to_collection
from error import generate_error
from sets import Set
from google.protobuf.descriptor import FieldDescriptor

class json_renderer:
    nearbyable_types = ['stop_points', 'stop_areas', 'coords', 'pois']
    scheduable_types = ['stop_points', 'stop_areas', 'coords', 'pois', 'lines', 'routes']
    pbtype_2_collection = {type_pb2.STOP_AREA : "stop_areas",
                           type_pb2.STOP_POINT : "stop_points"}

    def __init__(self, base_url):
        self.base_url = base_url
        self.visited_types = Set()

    def stop_point(self, obj, region, details=False, name_and_uri=True, administrative_regions=True):
        return self.generic_type('stop_points', obj, region, details, name_and_uri,administrative_regions)

    def stop_area(self, obj, region, details=False, name_and_uri=True):
        return self.generic_type('stop_areas', obj, region, details, name_and_uri)

    def route(self, obj, region, details=False):
        result = self.generic_type('routes', obj, region, details)
        try:
            if obj.HasField("is_frequence"):
                result["is_frequence"] = obj.is_frequence
        except:
            pass
        try:
            if obj.HasField("line"):
                result["line"] = self.line(obj.line, region, details)
        except:
            pass
        return result

    def line(self, obj, region, details=False):
        return self.generic_type('lines', obj, region, details)

    def network(self, obj, region, details=False):
        result = self.generic_type('networks', obj, region, details)
        try:
            result["lines"] = []
            for line in obj.lines:
                result["lines"].append(self.line(line, region, details))
        except:
            del result['lines']
        return result

    def commercial_mode(self, obj, region, details=False):
        result = self.generic_type('commercial_modes', obj, region, details)
        try:
            result["physical_modes"] = []
            for physical_mode in obj.physical_modes:
                result["physical_modes"].append(self.physical_mode(physical_mode, region, details))
        except:
            del result['physical_modes']
        return result


    def physical_mode(self, obj, region, details=False):
        result = self.generic_type('physical_modes', obj, region, details)
        try:
            result["commercial_modes"] = []
            for commercial_mode in obj.commercial_modes:
                result["commercial_modes"].append(self.commercial_mode(commercial_mode, region, details))
        except:
            del result['commercial_modes']
        return result

    def poi_type(self, obj, region, details=False):
        return self.generic_type("poi_types", obj, region, details)

    def poi(self, obj, region, details):
        result = self.generic_type('pois', obj, region, details)
        if obj.HasField("poi_type"):
            result["poi_type"] = self.poi_type('poi_types', obj.poi_type, region, details)
        return result



    def company(self, obj, region, details=False):
        return self.generic_type('companies', obj, region, details)

    def address(self, obj, region, details=False, name_and_uri=False):
        result = self.generic_type('addresses', obj, region, details, name_and_uri)
        if obj.HasField("house_number"):
            result["house_number"] = obj.house_number
        return result

    def administrative_region(self, obj, region, details):
        self.visited_types.add("administrative_regions")
        result = self.generic_type('administrative_regions', obj, region, details)
        try:
            if obj.HasField("zip_code"):
                result["zip_code"] = obj.zip_code
        except:
            pass
        try:
            if obj.HasField("level"):
                result["level"] = obj.level
        except:
            pass
        return result

    def stop_time(self, obj, region_name = None, details = False):
        result = {}
        result['date_time'] = obj.stop_time
        result['additional_information'] = []
        if obj.HasField("has_properties"):
            for additional_information in obj.has_properties.additional_informations:
                result['additional_information'].append(self.name_has_propertie(additional_information))

        r = []
        if obj.HasField("has_properties"):
            for note in obj.has_properties.notes:
                r.append({"id": note.uri, "type": "notes"})
        if (len(r)> 0):
            result["links"]= r

        if len(result['additional_information'])==0:
            del result['additional_information']
        return result

    def notes_stoptimes(self, obj, uri):
        r = []
        for row in obj.table.rows :
            if row.stop_times:
                for stop_time in row.stop_times:
                    if stop_time.HasField("has_properties"):
                        for note_ in stop_time.has_properties.notes:
                            r.append({"id": note_.uri, "value": note_.note})
        return r


    def route_schedule(self, obj, uri, region) :
        result = {'table' : {"headers" : [], "rows" : []}, "display_informations": [], "links":[]}

        for row in obj.table.rows :
            r = {}
            if row.stop_point:
                r['stop_point'] = self.stop_point(row.stop_point, region, False, True, False)
                self.visited_types.add("stop_point")
            if row.stop_times:
                r['date_times'] = []
                for stop_time in row.stop_times:
                    r['date_times'].append(self.stop_time(stop_time, region))
            result['table']['rows'].append(r)

        for header in obj.table.headers:
            result['table']['headers'].append(self.display_headers(header, region))

        if obj.HasField('route'):
            result['display_informations'] = self.route_informations(obj.route, region)

        if obj.HasField('route'):
            result['links'] = self.route_links(obj.route, region)

        if len(result['table']['headers'])==0:
            del result['table']['headers']

        if len(result['display_informations'])==0:
            del result['display_informations']
        return result

    def notes_stop_date_times(self, obj, uri):
        r = []
        for section in obj.sections:
            if section.stop_date_times:
                for stop_date_time in section.stop_date_times:
                    if stop_date_time.HasField('has_properties'):
                        for note_ in stop_date_time.has_properties.notes:
                            r.append({"id": note_.uri, "value": note_.note})
        return r

    def journey(self, obj, uri, details, is_isochrone, arguments):
        result = {
                'duration': obj.duration,
                'nb_transfers': obj.nb_transfers,
                'departure_date_time': obj.departure_date_time,
                'arrival_date_time': obj.arrival_date_time,
                'requested_date_time': obj.requested_date_time
                }
        if obj.HasField('origin'):
            self.visited_types.add("origin")
            result['origin'] = self.place(obj.origin, uri.region())
        if obj.HasField('destination'):
            self.visited_types.add("destination")
            result['destination'] = self.place(obj.destination, uri.region())

        if len(obj.sections) > 0:
            result['sections'] = []
            for section in obj.sections:
                result['sections'].append(self.section(section, uri.region()))

        if is_isochrone:
            params = "?"
            if obj.HasField('origin'):
                params = params + "from=" + obj.origin.uri + "&"
            else:
                params = params + "from=" + uri.uri + "&"
            if obj.HasField('destination'):
                resource_type = json_renderer.pbtype_2_collection[obj.destination.embedded_type]
                params = params + "to=" + uri.region() + "/" +resource_type +"/" + obj.destination.uri

            ignored_params = ["origin", "destination"]
            for k, v in arguments.givenByUser().iteritems():
                if not k in ignored_params:
                    params = params + "&" + k + "=" + v

            result["href"] = base_url + "/v1/journeys"+params
        return result

    def departures(self, obj):
        pass
        #result = {}

    def place(self, obj, region_name):
        obj_t = None
        result = {"name" :"", "id":"", "embedded_type" : ""}
        if obj.embedded_type == type_pb2.STOP_AREA:
            obj_t = obj.stop_area
            result["embedded_type"] = "stop_area"
            result["stop_area"] = self.stop_area(obj.stop_area, region_name, False, False)
        elif obj.embedded_type == type_pb2.STOP_POINT:
            obj_t = obj.stop_point
            result["embedded_type"] = "stop_point"
            result["stop_point"] = self.stop_point(obj.stop_point, region_name, False, False)
        elif obj.embedded_type  == type_pb2.ADDRESS:
            obj_t = obj.address
            result["embedded_type"] = "address"
            result["address"] = self.address(obj.address, region_name, False, False)
        if obj_t:
            #self.visited_types.add(result["embedded_type"])
            result["name"] = obj_t.name
            result["id"] = region_name + "/" + resource_type_to_collection[result["embedded_type"]]+"/"+obj.uri

        return result

    def time_place(self, obj, region_name, is_from=True):
        obj_t = None
        result = {}
        if is_from:
            obj_oridest = obj.destination
            if obj.HasField('begin_date_time'):
                result['date_time'] = obj.begin_date_time
        else:
            obj_oridest = obj.origin
            if obj.HasField('end_date_time'):
                result['date_time'] = obj.end_date_time

        if obj_oridest.embedded_type == type_pb2.STOP_AREA:
            obj_t = obj_oridest.stop_area
            result["embedded_type"] = "stop_area"
            result["stop_area"] = self.stop_area(obj_oridest.stop_area, region_name, True)
        elif obj_oridest.embedded_type == type_pb2.STOP_POINT:
            obj_t = obj_oridest.stop_point
            result["embedded_type"] = "stop_point"
            result["stop_point"] = self.stop_point(obj_oridest.stop_point, region_name, True)
            result["stop_point"]["stop_area"] = self.stop_area(obj_oridest.stop_point.stop_area, region_name, True)
        elif obj_oridest.embedded_type  == type_pb2.ADDRESS:
            obj_t = obj_oridest.address
            result["embedded_type"] = "address"
            result["address"] = self.address(obj_oridest.address, region_name, True)
        if obj_t:
            #self.visited_types.add(result["embedded_type"])
            result["name"] = obj_t.name
            result["id"] = region_name + "/" + resource_type_to_collection[result["embedded_type"]]+"/"+obj_oridest.uri

        return result

    def region(self, obj, region_id, details=False):
        result = {
                'id': region_id,
                'start_production_date': obj.start_production_date,
                'end_production_date': obj.end_production_date,
                'status': 'running',
                'shape': obj.shape
                }
        return result


    def generic_type(self, type, obj, region, details = False, name_and_uri=True, administrative_regions=True):
        result = {}
        if name_and_uri : 
            result['name'] = obj.name
            result['id'] = region + '/' + type + '/' + obj.uri
        try: # si jamais y’a pas de champs coord dans le .proto, HasField gère une exception
            if obj.HasField('coord'):
                result['coord'] = {'lon': obj.coord.lon, 'lat': obj.coord.lat}
        except:
            pass
        try:
            result['administrative_regions'] = []
            if administrative_regions :
                for admin in obj.administrative_regions:
                    result['administrative_regions'].append(self.administrative_region(admin, region, False))
        except:
            pass
        if len(result['administrative_regions'])==0:
            del result['administrative_regions']
        return result

    def section_links(self, region_name, uris):
        links = []
        if uris.HasField('company'):
            links.append({'type' : 'company' , 'id' : region_name + '/companies/' + uris.company})

        if uris.HasField('vehicle_journey'):
            links.append({'type' : 'vehicle_journey' , 'id' : region_name + '/vehicle_journeys/' + uris.vehicle_journey})

        if uris.HasField('line'):
            links.append({'type' : 'line' , 'id' : region_name + '/lines/' + uris.line})

        if uris.HasField('route'):
            links.append({'type' : 'route' , 'id' : region_name + '/routes/' + uris.route})

        if uris.HasField('commercial_mode'):
            links.append({'type' : 'commercial_mode' , 'id' : region_name + '/commercial_modes/' + uris.commercial_mode})

        if uris.HasField('physical_mode'):
            links.append({'type' : 'physical_mode' , 'id' : region_name + '/physical_modes/' + uris.physical_mode})

        if uris.HasField('network'):
            links.append({'type' : 'network' , 'id' : region_name + '/networks/' + uris.network})
                       
        return links

    def display_headers(self, header, region_name):
        result = {}
        links = []
        equipments = []
        links.append({"type": "vehicle_journey", "id": region_name + "/vehicles_journeys/" + header.vehiclejourney.uri, "templated":"false"})
        self.visited_types.add("vehicle_journey")
        if(len(header.vehiclejourney.name) > 0):
            result['headsign'] = header.vehiclejourney.name
        if(len(header.direction) > 0):
            result['direction'] = header.direction
        if(len(header.vehiclejourney.physical_mode.name) > 0):
            result['physical_mode'] = header.vehiclejourney.physical_mode.name
            links.append({"type": "physical_mode", "id": region_name + "/physical_modes/" + header.vehiclejourney.physical_mode.uri, "templated":"false"})
            self.visited_types.add("physical_mode")
        if(len(header.vehiclejourney.odt_message) > 0):
            result['description'] = header.vehiclejourney.odt_message
        if header.vehiclejourney.wheelchair_accessible:
            result['wheelchair_accessible'] = "true"
        if header.vehiclejourney.bike_accepted:
            result['bike_accessible'] = "true"
        if header.vehiclejourney.visual_announcement:
            equipments.append({"id": "has_visual_announcement"})
        if header.vehiclejourney.audible_announcement:
            equipments.append({"id": "has_audible_announcement"})
        if header.vehiclejourney.appropriate_escort:
            equipments.append({"id": "has_appropriate_escort"})
        if header.vehiclejourney.appropriate_signage:
            equipments.append({"id": "has_appropriate_signage"})
        if (len(equipments)> 0):
            result["equipments"] = equipments
        result["links"] = links
        return result

    def route_links(self, rte, region_name):
        links = []
        links.append({"type": "line", "id": region_name + "/lines/" + rte.line.uri})
        self.visited_types.add("line")
        links.append({"type": "route", "id": region_name + "/routes/" +  rte.uri})
        self.visited_types.add("route")
        if(len(rte.line.network.name) > 0):
            links.append({"type": "network", "id": region_name + "/networks/" + rte.line.network.uri})
            self.visited_types.add("network")
        if(len(rte.line.commercial_mode.name) > 0):
            links.append({"type": "commercial_mode", "id": region_name + "/commercial_modes/" + rte.line.commercial_mode.uri})
            self.visited_types.add("commercial_mode")
        #if(len(rte.line.physical_mode.name) > 0):
        #    links.append({"type": "physical_mode", "id": region_name + "/physical_modes/" + rte.line.commercial_mode.uri})
        #    self.visited_types.add("physical_mode")
        return links

    def route_informations(self, rte, region_name):
        result = {}
        if(len(rte.line.network.name) > 0):
            result['network'] = rte.line.network.name
        if(len(rte.line.code) > 0):
            result['code'] = rte.line.code
        if(len(rte.name) > 0):
            result['headsign'] = rte.name
        if(len(rte.line.color) > 0):
            result['color'] = rte.line.color
        if(len(rte.line.commercial_mode.name) > 0):
            result['commercial_mode'] = rte.line.commercial_mode.name
        #if(len(rte.line.physical_mode.name) > 0):
        #    result['physical_mode'] = rte.line.commercial_mode.name
        return result

    def display_informations(self, infos, region_name):
        result = {}

        if(len(infos.network) > 0):
            result['network'] = infos.network
        if(len(infos.code) > 0):
            result['code'] = infos.code
        if(len(infos.headsign) > 0):
            result['headsign'] = infos.headsign
        if(len(infos.color) > 0):
            result['color'] = infos.color
        if(len(infos.commercial_mode) > 0):
            result['commercial_mode'] = infos.commercial_mode
        if(len(infos.physical_mode) > 0):
            result['physical_mode'] = infos.physical_mode
        if(len(infos.direction) > 0):
            result['direction'] = infos.direction
        if(len(infos.description) > 0):
            result['description'] = infos.description
        return result

    def street_network(self, street_network):
        result = {
                'path_items': [],
                'coordinates': []
                }
        for item in street_network.path_items:
            result['path_items'].append({'name': item.name, 'length': item.length})

        for coord in street_network.coordinates:
            result['coordinates'].append({'lon': coord.lon, 'lat': coord.lat})

        return result

    def name_has_propertie(self, enum):
        descriptor = type_pb2.hasPropertie.DESCRIPTOR.enum_types_by_name['AdditionalInformation'].values_by_number
        return descriptor[enum].name

    def stop_date_time(self, obj, region_name):
        result = {}
        if obj.HasField('departure_date_time'):
            result['departure_date_time'] = obj.departure_date_time

        if obj.HasField('arrival_date_time'):
            result['arrival_date_time'] = obj.arrival_date_time

        if obj.HasField('stop_point'):
            self.visited_types.add("stop_point")
            result['stop_point'] = self.stop_point(obj.stop_point, region_name)

        if obj.HasField('has_properties'):
            result['additional_information'] = []
            for additional_information in obj.has_properties.additional_informations:
                result['additional_information'].append(self.name_has_propertie(additional_information))

        if obj.HasField('has_properties'):
            result['notes'] = []
            for note_ in obj.has_properties.notes:
                result['notes'].append({"id": note_.uri})

        return result
    def estimated_duration(self, stop_date_times):
        result = False
        for stop_dt in stop_date_times:
            if stop_dt.HasField('has_properties'):
                for additional_information in stop_dt.has_properties.additional_informations:
                    if additional_information == type_pb2.hasPropertie.DATE_TIME_ESTIMATED:
                        result = True
                        break
        return result

    def section(self, obj, region_name):
        self.visited_types.add("origin")
        self.visited_types.add("destination")
        if obj.pt_display_informations.odt_type != type_pb2.regular_line:
            section_type = "on_demand_transport"
        else :
            section_type = get_name_enum(obj, obj.type)

        result = {'type': section_type,
                  'duration': obj.duration,
                  'from': self.time_place(obj, region_name),
                  'to': self.time_place(obj, region_name, False)}
        #if obj.pt_display_informations.odt_type != type_pb2.regular_line:
            #result["odt_type"] = get_name_enum(obj.pt_display_informations, obj.pt_display_informations.odt_type)
        result['additional_information'] = []
        if self.estimated_duration(obj.stop_date_times):
            result['additional_information'].append("duration_estimated")
        if obj.HasField('uris'):
            result['links'] = self.section_links(region_name, obj.uris)
        if obj.HasField('pt_display_informations'):
            result['display_informations'] = self.display_informations(obj.pt_display_informations, region_name)

        if len(obj.stop_date_times) > 0:
            result['stop_date_times'] = []
            for stop_dt in obj.stop_date_times:
                result['stop_date_times'].append(self.stop_date_time(stop_dt, region_name))

        if obj.HasField('street_network'):
            result['street_network'] = self.street_network(obj.street_network)
            result['length'] = obj.street_network.length

        if obj.HasField('transfer_type'):
            result['transfert_type'] = obj.transfer_type

        return result

    def passage(self, obj, region_name):
        result = {
                'stop_date_time': self.stop_date_time(obj.stop_date_time, region_name),
                'stop_point': self.stop_point(obj.stop_point, region_name),
                }
        try:
            result['route'] = self.route(obj.vehicle_journey.route, region_name)
        except:
            del result['route']
            pass
        try:
            if obj.HasField('pt_display_informations'):
                result['pt_display_informations'] = self.display_informations(obj.pt_display_informations, region_name)
        except:
            pass
        return result

    def link_types(self, region_name):
        t_url = base_url + "/v1/coverage/{"
        result = []
        for t in self.visited_types:
            result.append({"href":t_url+t+".id}", "rel": t, "templated":"true"})
        return result




def get_field_by_name(obj, name):
    for field_tuple in obj.ListFields():
        if field_tuple[0].name == name:
            return field_tuple[1] 
    return None

def pagination(base_url, obj):
    result = {}
    result['total_result'] = obj.totalResult
    result['current_page'] = obj.startPage
    result['items_on_page'] = obj.itemsOnPage
    return result

def pagination_links(base_url, obj):
    result = []
    result.append({'href' : base_url, 'rel' : 'first', "templated":False})
    if obj.itemsOnPage > 0:
        result.append({'href' : base_url + '?start_page=' +
                       str(int(obj.totalResult/obj.itemsOnPage)), "rel" : "last", "templated":False})
    else:
        result.append({'href' : base_url, "rel" : "last", "templated":False})
    if obj.HasField('nextPage'):
        result.append({'href' : base_url + '?start_page=' + str(obj.startPage + 1), "rel" : "next", "templated":False})
    if obj.HasField('previousPage'):
        result.append({'href' : base_url + '?start_page=' + str(obj.startPage - 1), "rel" : "prev", "templated":False})
    return result


def render_ptref(response, region, resource_type, uid, format, callback):
    if response.error and response.error == '404':
        return generate_error("No object found", status=404)
    
    resp_dict = dict([(resource_type, []), ("links", []), ("pagination", {})])
    resp_dict[resource_type] = []
    renderer = json_renderer(base_url + '/v1/coverage')
    renderer.visited_types.add(resource_type)
    items = get_field_by_name(response, resource_type)
    if items:
        for item in items:
            resp_dict[resource_type].append(renderer.generic_type(resource_type, item, region))
    resp_dict['pagination'] = pagination(base_url + '/v1/coverage/' + region + '/' + resource_type, response.pagination)

    if not uid:
        resp_dict['links'] = pagination_links(base_url+ '/v1/coverage/' + region + '/' + resource_type , response.pagination)
    
    if uid:
        link_first_part = base_url+"/v1/coverage/"+ region+"/"+resource_type+"/"+uid
        if resource_type in json_renderer.nearbyable_types:
            resp_dict['links'].append({"href" : link_first_part+"/journeys", "rel":"journeys", "templated":False})
            resp_dict['links'].append({"href" : link_first_part+"/places_nearby", "rel":"places_nearby", "templated":False})
        if resource_type in json_renderer.scheduable_types:
            resp_dict['links'].append({"href" : link_first_part+"/route_schedules", "rel":"route_schedules", "templated":False})
            #resp_dict['curies'].append({"href" : base_url+"/v1/coverage/{"+resource_type+".id/stop_schedules", "rel":"stop_schedules"})
            resp_dict['links'].append({"href" : link_first_part+"/departures", "rel":"departures", "templated":False})
            resp_dict['links'].append({"href" : link_first_part+"/arrivals", "rel":"arrivals", "templated":False})
        for key in collections_to_resource_type:
            if key != type:
                resp_dict['links'].append({"href" : link_first_part+"/"+key, "rel":""+key, "templated":False}) 
    else:
        resp_dict['links'].append({"href" : base_url + "/v1/coverage/{"+resource_type+".id}", "rel" : "related", "templated":True})

    resp_dict['links'].extend(renderer.link_types(region))
    return render(resp_dict, format, callback)

def coverage(request, region_name=None, format=None):
    region_template = "{regions.id}"
    if region_name:
        region_template = region_name
    result = {'regions': [], 
              'links' : []}

    links =  [{"href" : base_url +"/v1/coverage/"+region_template, "rel":"related"}]

    for key in collections_to_resource_type:
        links.append({"href" : base_url+"/v1/coverage/"+region_template+"/"+key, "rel":""+key})

    result['links'] = links

    for link in result['links']:
        link["templated"] = region_name==None
    
    if not region_name:
        regions = NavitiaManager().instances.keys() 
    else:
        regions = {region_name}
    
    renderer = json_renderer(base_url + '/v1/')
    req = request_pb2.Request()
    req.requested_api = type_pb2.METADATAS
    for r_name in regions:
        try:
            resp = NavitiaManager().send_and_receive(req, r_name) 
            result['regions'].append(renderer.region(resp.metadatas, r_name))
        except DeadSocketException :
            if region_name:
                return generate_error('region : ' + r_name + ' is dead', 500)
            else :
                result['regions'].append({"id" : r_name, "status" : "not running", "href": base_url + "/v1/" + r_name})
        except RegionNotFound:
            if region_name:
                return generate_error('region : ' + r_name + " not found ", 404)
            else: 
                result['regions'].append({"id" : r_name, "status" : "not found", "href": base_url + "/v1/" + r_name})
    
    return render(result, format,  request.args.get('callback'))

def coord(request, lon_, lat_):
    try:
        lon = float(lon_)
        lat = float(lat_)
    except ValueError:
        return generate_error("Invalid coordinate : " +lon_+":"+lat_, 400)

    result_dict = {"coord" : {"lon":lon, "lat": lat, "regions" : []}, "links":[]}
    
    region_key = NavitiaManager().key_of_coord(lon, lat)
    if(region_key):
        result_dict["coord"]["regions"].append({"id":region_key})

    result_dict["links"].append({"href":base_url + "/v1/coverage/coord/"+lon_+";"+lat_+"/journeys", "rel" :"journeys", "templated":False})
    result_dict["links"].append({"href":base_url + "/v1/coverage/coord/"+lon_+";"+lat_+"/places_nearby", "rel" :"nearby", "templated":False})
    result_dict["links"].append({"href":base_url + "/v1/coverage/coord/"+lon_+";"+lat_+"/departures", "rel" :"departures", "templated":False})
    result_dict["links"].append({"href":base_url + "/v1/coverage/coord/"+lon_+";"+lat_+"/arrivals", "rel" :"arrivals", "templated":False})
    result_dict["links"].append({"href":"www.openstreetmap.org/?mlon="+lon_+"&mlat="+lat_+"&zoom=11&layers=M", "rel":"about", "templated":False})

    return render(result_dict, "json", request.args.get('callback'))


def index(request, format='json'):
    response = {
            "links" : [
                    {"href" : base_url + "/v1/coverage", "rel" :"coverage", "title" : "Coverage of navitia"},
                    {"href" : base_url + "/v1/coord", "rel" : "coord", "title" : "Inverted geocooding" },
                    {"href" : base_url + "/v1/journeys", "rel" : "journeys", "title" : "Compute journeys"}
                    ]  
            }
    return render(response, format, request.args.get('callback'))

def reconstruct_pagination_journeys(string, region_name):
    args = []

    for arg_and_val in string.split("&"):
        if len(arg_and_val.split("=")) == 2:
            arg, val = arg_and_val.split("=")
            if arg == "origin" or arg == "destination":
                coord = ""
                if val[:5] == "coord":
                    val = val[5:-1]
                    resource_type = "coord"
                else:
                    resource_type = val.split(":")[0]
                val = region_name + "/" + resource_type_to_collection[resource_type] + "/" + val

                if arg == "origin":
                    arg = "from"
                else:
                    arg = "to"

            if arg == "clockwise":
                if val == "true":
                    arg = "datetime_represents"
                    val = "departure"
                else :
                    arg = "datetime_represents"
                    val = "arrival"

            args.append(arg+"="+val)
    return "&".join(args)

def get_name_enum(obj, enum):
    for field, value in obj.ListFields():
        if field.type == FieldDescriptor.TYPE_ENUM and enum == value:
            return field.enum_type.values_by_number[int(value)].name
    return ""

def street_network_display_informations(journey) :
    for section in journey.sections:
        if section.type == response_pb2.STREET_NETWORK:
            if section.HasField('street_network'):
                section.pt_display_informations.physical_mode = get_name_enum(section.street_network, section.street_network.mode)
                section.pt_display_informations.direction = section.street_network.path_items[-1].name



def journeys(arguments, uri, response, format, callback, is_isochrone=False):
    renderer = json_renderer(base_url + '/v1/')
    if is_isochrone:
	    response_dict = {'journeys': [], "links" : [], 'notes' : []}
    if not is_isochrone:
        response_dict = {'pagination': {'links' : []}, 'response_type' : '', 'journeys': [], 'notes' : []}
    for journey in response.journeys:
        street_network_display_informations(journey)
        response_dict['journeys'].append(renderer.journey(journey, uri, True, is_isochrone, arguments))
        response_dict['notes'].extend(renderer.notes_stop_date_times( journey, uri))
    if is_isochrone:
        response_dict['links'].extend(renderer.link_types(uri.region()))
    if not is_isochrone:
        prev = reconstruct_pagination_journeys(response.prev, uri.region())
        next = reconstruct_pagination_journeys(response.next, uri.region())
        response_dict['response_type'] = get_name_enum(response, response.response_type)
        response_dict['pagination']['links'].append({"href":base_url+"/v1/journeys?"+next, "type":"next","templated":False })
        response_dict['pagination']['links'].append({"href":base_url+"/v1/journeys?"+prev, "type":"prev","templated":False })

    return render(response_dict, format, callback)

def departures(response, region, format, callback):
    renderer = json_renderer(base_url + '/v1/')
    renderer.visited_types.add('departures')
    response_dict = {'departures': [], "links" : [], "pagination" : {}}
    response_dict['pagination']['total_result'] = len(response.places)
    response_dict['pagination']['current_page'] = 0
    response_dict['pagination']['items_on_page'] = len(response.places) 

    for passage in response.next_departures:
        response_dict['departures'].append(renderer.passage(passage, region))
    response_dict['links'] = renderer.link_types(region)
    return render(response_dict, format, callback)

def arrivals(response, region, format, callback):
    renderer = json_renderer(base_url + '/v1/')
    renderer.visited_types.add('arrivals')
    response_dict = {'arrivals': [], "pagination": {}, "links" : []}
    response_dict['pagination']['total_result'] = len(response.next_arrivals)
    response_dict['pagination']['current_page'] = 0
    response_dict['pagination']['items_on_page'] = len(response.next_arrivals) 
    for passage in response.next_arrivals:
        response_dict['arrivals'].append(renderer.passage(passage, region))
    response_dict['links'] = renderer.link_types(region)
    return render(response_dict, format, callback)

def places(response, uri, format, callback):
    renderer = json_renderer(base_url + '/v1/')
    renderer.visited_types.add("places")
    response_dict = {"links" : [], "pagination" : {}, "places" : []}

    response_dict['pagination']['total_result'] = len(response.next_departures)
    response_dict['pagination']['current_page'] = 0
    response_dict['pagination']['items_on_page'] = len(response.next_departures) 
    for place in response.places:
        response_dict['places'].append(renderer.place(place, uri.region()))
        response_dict['places'][-1]['quality'] = place.quality
    
    response_dict['links'].append({"href" : base_url+"/v1/coverage/{places.id}",
                                   "rel" : "places.id",
                                   "templated" : True})
    
    return render(response_dict, format, callback)
    

def nearby(response, uri, format, callback):
    renderer = json_renderer(base_url + '/v1/')
    renderer.visited_types.add("places_nearby")
    response_dict = {"links" : [], "pagination" : {}, "places_nearby" : []}
    response_dict['pagination']['total_result'] = len(response.places_nearby)
    response_dict['pagination']['current_page'] = 0
    response_dict['pagination']['items_on_page'] = len(response.places_nearby) 
    for place in response.places_nearby:
        response_dict['places_nearby'].append(renderer.place(place, uri.region()))
        response_dict['places_nearby'][-1]['distance'] = place.distance
    
    response_dict['links'] = renderer.link_types(uri.region())
    return render(response_dict, format, callback)

def route_schedules(response, uri, region, format, callback):
    renderer = json_renderer(base_url+'/v1/coverage/')
    response_dict = {"links" : [], "route_schedules" : [], "notes":[]}

    for schedule in response.route_schedules:
        response_dict['route_schedules'].append(renderer.route_schedule(schedule, uri, region))
        response_dict['notes'].extend(renderer.notes_stoptimes(schedule, uri))

    response_dict['links'].extend(renderer.link_types(region))
    return render(response_dict, format, callback)
