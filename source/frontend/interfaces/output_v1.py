

def add_uri(collection_name, region, collection):
    for item in collection:
        item.uri = region+"/"+collection_name+"/"+item.uri
    return collection


def add_uri_resource(collection_name, region, resource):
    resource.uri = region+"/"+collection_name+"/"+resource.uri
