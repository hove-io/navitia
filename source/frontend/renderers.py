# coding=utf-8
import type_pb2
import json
import dict2xml
from protobuf_to_dict import protobuf_to_dict
from werkzeug.wrappers import Request, Response


def render(dico, format, callback):
    if format == 'json':
        json_str = json.dumps(dico, ensure_ascii=False)
        if callback == '' or callback == None:
            result = Response(json_str, mimetype='application/json')
        else:
            result = Response(callback + '(' + json_str + ')', mimetype='application/json;charset=utf-8')
    elif format == 'txt':
        result = Response(json.dumps(dico, ensure_ascii=False, indent=4), mimetype='text/plain;charset=utf-8')
    elif format == 'xml':
        result = Response('<?xml version="1.0" encoding="UTF-8"?>\n'+ dict2xml.dict2xml(dico, wrap="Response"), mimetype='application/xml;charset=utf-8')
    elif format == 'pb':
        result = Response('Protocol buffer not supported for this request', status=404)
    else:
        result = Response("Unknown file format format. Please choose .json, .txt, .xml or .pb", mimetype='text/plain;charset=utf-8', status=404)
    result.headers.add('Access-Control-Allow-Origin', '*')
    return result


def render_from_protobuf(pb_resp, format, callback):
    if format == 'pb':
        return Response(pb_resp.SerializeToString(), mimetype='application/octet-stream')
    else:
        return render(protobuf_to_dict(pb_resp, use_enum_labels=True), format, callback)
