// randomSPQ.click

//output 0 is hihg, 1 is low
ipclass :: IPClassifier(src udp port 0, src udp port 1, -);

out :: Queue(10000000)->BandwidthShaper(1000000)->
        //Print("->eth1", CONTENTS 'NONE')->
    	ToDevice(eth1);
		
high :: Queue(10000000)
low :: Queue(10000000)

//output 0 is high, 1 is low, the probability of high is 0.2 
randomClass :: RandomPacketClassifier(0.2);

FromDevice(eth0, PROMISC true)->
        //Print("eth0->", CONTENTS 'NONE')->
	MarkIPHeader()->ipclass;

	ipclass[0]->high;
	ipclass[1]->randomClass;
	
	randomClass[0]->high;
	randomClass[1]->low;
	
	PrioSched(high, low)->out;

FromDevice(eth1, PROMISC true)->
	//Print("eth1->", CONTENTS 'NONE')->
	Queue->
		//Print("->eth0", CONTENTS 'NONE')->
       		ToDevice(eth0);