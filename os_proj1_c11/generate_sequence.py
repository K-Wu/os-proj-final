import numpy as np
import sys
argv=sys.argv
NUM_CUSTOMERS=100
FILENAME="data.dat"
if len(argv)==3:
    NUM_CUSTOMERS=int(argv[1])
    FILENAME=argv[2]
entering_time=np.random.random_integers(0,10,NUM_CUSTOMERS)
waiting_time=np.random.random_integers(1,40,NUM_CUSTOMERS)
seq=[]
for i in range(1,NUM_CUSTOMERS+1):
    seq.append((i,entering_time[i-1],waiting_time[i-1]))
with open(FILENAME,'w') as fd:
    for id,ent,wait in seq:
        fd.write(str(id)+" "+str(ent)+" "+str(wait)+'\n')
