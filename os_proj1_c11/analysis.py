import sys
done=0
wait_lock_set=set()
FILENAME=sys.argv[1]
served_customer_set=set()
with open(FILENAME,'r') as fd:
    for line in fd:
        if line.find("done")!=-1:
            done+=1
        elif line.find("served")!=-1:
            customer_data_id=line[line.find("data_id")+9:line.find("served")-4]
            customer_data_id=int(customer_data_id)
            if customer_data_id in served_customer_set:
                print("duplicate serving infomation")
            served_customer_set.add(customer_data_id)
        elif line.find("waiting")!=-1:
            customer_id=line[line.find("data_id")+8:line.find("waiting")-1]
            customer_id=int(customer_id)
            if customer_id in wait_lock_set:
                print("duplicate waiting customer")
            wait_lock_set.add(customer_id)
if done!=len(wait_lock_set) or done!=len(served_customer_set):
    print("err in nums")
    print(done,len(wait_lock_set),len(served_customer_set))
