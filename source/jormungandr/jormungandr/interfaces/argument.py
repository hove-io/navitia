from flask.ext.restful import reqparse
import six

class ArgumentDoc(reqparse.Argument):
    def __init__(self, name, default=None, dest=None, required=False,
                 ignore=False, type=six.text_type, location=('values',),
                 choices=(), action='store', help=None, operators=('=',),
                 case_sensitive=True, description=None, hidden=False):
        super(ArgumentDoc, self).__init__(name, default, dest, required, ignore,
                                          type, location, choices, action, help,
                                          operators, case_sensitive)
        self.description = description
        self.hidden = hidden
