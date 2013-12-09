from jormungandr.instance import Instance
import os
import hashlib


class InstanceTest(Instance):
    def __init__(self, saving_directory, instance):
        self.geom = instance.geom
        self._sockets = instance._sockets
        self.socket_path = instance.socket_path
        self.script = instance.script
        self.nb_created_socket = instance.nb_created_socket
        self.lock = instance.lock
        self.context = instance.context
        self.name = instance.name
        self.saving_directory = saving_directory

    def make_filename(self, request):
        filename = hashlib.md5(str(request).encode()).hexdigest()
        return os.path.join(self.saving_directory, filename)
