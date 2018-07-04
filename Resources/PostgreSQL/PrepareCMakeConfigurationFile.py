#!/usr/bin/python

import re
import sys

if len(sys.argv) != 3:
    raise Exception('Bad number of arguments')

r = re.compile(r'^#undef ([A-Z0-9_]+)$')

with open(sys.argv[1], 'r') as f:
    with open(sys.argv[2], 'w') as g:
        for l in f.readlines():
            m = r.match(l)
            if m != None:
                s = m.group(1)
                g.write('#cmakedefine %s @%s@\n' % (s, s))
            else:
                g.write(l)
                
