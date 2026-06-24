import collections
import collections.abc
import sys

# Python 3.10 removed direct access to ABCs from the collections module.
# Some dependencies (e.g. flex 6.10.0) still import from collections directly.
# Restore the aliases so these packages work on Python 3.10+.
if sys.version_info >= (3, 10):
    for attr in ('Callable', 'Mapping', 'MutableMapping', 'Sequence', 'MutableSequence'):
        if not hasattr(collections, attr):
            setattr(collections, attr, getattr(collections.abc, attr))
