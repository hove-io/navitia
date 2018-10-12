#!/usr/bin/env python
import sys

f = open(sys.argv[1], 'r')
source = f.readlines()
source[0] = source[0][source[0].index(":") :]
source[0] = "Source" + source[0]
f.close()
f = open(sys.argv[2], 'w')
f.writelines(source)
f.write('\n')
f.close()
f = open(sys.argv[1], 'r')
package = f.readlines()
f.close()
f = open(sys.argv[2], 'a')
f.writelines(package)
f.close()
