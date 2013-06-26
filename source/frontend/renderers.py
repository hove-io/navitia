# coding=utf-8
import json
import dict2xml
from protobuf_to_dict import protobuf_to_dict
from werkzeug.wrappers import Response
from error import generate_error


def render(dico, format, callback):
    if format == 'application/json':
        json_str = json.dumps(dico, ensure_ascii=False)
        if callback == '' or callback == None:
            result = Response(json_str, mimetype='application/json;charset=utf-8')
        else:
            result = Response(callback + '(' + json_str + ')', mimetype='application/json;charset=utf-8')
    elif format == 'text/html' or format == 'text/plain':
        json_str = json.dumps(add_links(dico), ensure_ascii=False, indent=4)
        result = Response('<html><pre>' + json_str + '</pre></html>', mimetype='text/html;charser=utf8')
    elif format == 'txt':
        result = Response(json.dumps(dico, ensure_ascii=False, indent=4), mimetype='text/plain;charset=utf-8')
    elif format == 'xml':
        result = Response('<?xml version="1.0" encoding="UTF-8"?>\n'+ dict2xml.dict2xml(dico, wrap="Response"), mimetype='application/xml;charset=utf-8')
    elif format == 'pb':
        result = generate_error('Protocol buffer not supported for this request', status=404)
    else:
        result = generate_error("Unknown file format format('"+format+"'). Please choose .json, .txt, .xml or .pb", status=404)
    result.headers.add('Access-Control-Allow-Origin', '*')
    return result

def search_links(dico):
    result = {}
    if dico.has_key("links") : 
        for link in dico['links']:
            if 'templated' in link and link['templated'] : 
                if link['rel'] == "related":
                    for key, val in dico.iteritems():
                        if key!="links" and key !="pagination":
                            result[key+".id"] = link['href']
                else:
                    result[link['rel']] = link['href']
    print "links : ",result
    return result

def add_a(obj, links, last_type):
    if last_type in links:
        result = "<a href='"
        result+= links[last_type].replace("{"+last_type+"}", obj)
        result+= "'>"+obj+"</a>"
        return result
    else :
        return obj

def add_links(obj):
    links = search_links(obj)
    result = add_links_recc(obj, links)
    
    if obj.has_key("links"):
        for link in obj["links"]:
            if 'templated' in link and not link['templated']:
                link['href'] = "<a href='"+link['href']+"'>"+link['href']+'</a>'
    return result


def add_links_recc(obj, links, last_type=None):
    if type(obj) == type({}):
        for key, value in obj.iteritems():
            object_type = last_type 
            if key=="id":
                obj[key] = add_a(value, links, object_type)
            if key=="href" and last_type != "links":
                obj[key] = "<a href='"+obj[key]+"'>"+obj[key]+'</a>'
            add_links_recc(obj[key], links, key)
    elif type(obj) == type([]):
        for value in obj:
            add_links_recc(value, links, last_type)
    return obj

def render_from_protobuf(pb_resp, format, callback):
    if format == 'pb':
        return Response(pb_resp.SerializeToString(), mimetype='application/octet-stream')
    else:
        return render(protobuf_to_dict(pb_resp, use_enum_labels=True), format, callback)
