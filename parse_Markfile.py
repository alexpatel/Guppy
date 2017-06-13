# ASSUMES EXISTENCE OF MARKFILE THAT IS
# GENERATED BY HAKE/MAIN.HS FROM THE SAME COMMIT.
# CREATES parsed_Markfile.txt with alternating lines:
#     TARGET
#     DEPENDENCY

# THE OUTPUT OF THIS FILE SHOULD BE RUN AGAINST
# A BUILD'S MENU.LST TO DETERMINE THE LIST
# OF SOURCE FILES INVOLVED WITH A PARTICULAR
# RUN OF BARRELFISH

# I SAY *RUN* AND NOT *BUILD*  BECAUSE
# BY DEFAULT, ALL FILES IN THE SOURCE
# TREE WILL BE BUILT. THE MENU.LST
# JUST DETERMINES WHICH WILL *RUN*

import sys
filename = "Markfile"

def clean(s):
    s = s.strip("")
    s = s.rstrip()
    return s

lines = None
CUR_LINE = 0
target_deps = {}

with open(filename, "r") as f:
    lines = f.readlines()
NUM_LINES = len(lines)

while CUR_LINE < NUM_LINES-1:
    assert "OUTPUTS" in lines[CUR_LINE]
    
    output = clean(lines[CUR_LINE+1])
    deps = clean(lines[CUR_LINE+4])
    predeps = clean(lines[CUR_LINE+7])


    key = output
    val = filter(None,[deps,predeps])
    val = [item for segments in val for item in segments.split()]
    target_deps[key] = val
    
    CUR_LINE += 9

with open('parsed_Markfile.txt', 'w') as f:
    for k,v in target_deps.iteritems():
        f.write("TARGET: "+k+'\n')
        f.write("DEPS:"+(','.join(v))+'\n')
