from werkzeug.wrappers import Response
import json

def generate_error(str, status=400):
    return Response(json.dumps({"error" : str}),mimetype='application/json;charset=utf-8', status=status)
