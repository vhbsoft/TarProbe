## QOSSender Configuration
<html>
	
	host 127.0.0.1   //default host
	port 55555   //default port
	set m  	//declare a variable m
	set n		// the variable name can't contain any of "[]/=," nor space 
	size   1024      //default packet size
	//seed 0    //use a fixed random seed, otherwise the seed is time(0)
	repeat m = [100, 200]    //repeat the following block for 100 to 200 times
		size 1000   //overwrite the packet size to 1000
		port 55556   // overwrite the port to 55556, which is the high priority port, the host will be the default
	end

	repeat 10 //repeat the following block for 10 times
		repeat 1   //send 1 low priority packet
			port 55555 //overwirte the port, the size and host will be the default
		end
		repeat n = [1,5] //send 1 to 5 high priority packets, also change the n
			port 55556 //overwirte the port, the size and host will be the default
		end
	end

	repeat 15 //repeat the following block for 10 times
		repeat n   //send n low priority packet, the value of n is the current value of n
			port 55555 //overwirte the port, the size and host will be the default
		end
		repeat n[] //send n high priority packets, the value is the sequence of n, loop back if exceed the boundary
			port 55556 //overwirte the port, the size and host will be the default
		end
	end
	//By nesting loops, you can send any sequence 
	//any line doesn't start with "host", "port", "size", "repeat", "set", "seed" or "end" will be ignored
	//the parameters will be in the "output.txt", so you can know the random values
	
</html>

Above is an example of the configuration.  
There are 7 commands you can use in a configuration. Each command is a line with at most 2 strings separated by space. If there is a second string, then the second string is the parameter.  
A parameter can be a constant or a variable.
1. host: This specifics the host ip address. You can't use domain name, must be an IP v4 address. If this command is used outside of a loop, you are specificing the default host.
2. port: This specifics the destination UDP port. If this command is used outside of a loop, you are specificing the default port.
3. size: This specifics packet's size in byte. If this command is used outside of a loop, you are specificing the default packet size.
4. repeat: This starts a loop. The parameter is times of the loop. You can nest loops. And also can overwrite host, port and size by call these commands in a loop. Note that only the loop without any nested loop can overwrite these values.
5. end: This ends a loop. It doesn't have any parameter.
6. set: This declares a variable. The parameter is the variable's name, and maybe with the value.
7. seed: This specifics the random seed you want to use. This command always set the global seed wherever it is used.

There are 4 ways to declare or use a variable. For example, the variable name is n:
1. Just the name, like "n": this way, you don't change the value of n. If this is used for declaring, n has a default value which is 0.
2. Name = value, like "n = 10": this way, you set n with the value 10.
3. Name = a range of value, like "n = [1， 10]": this way, you set n with a value randomly choose from 1 to 10, both boundaries are included.
4. Name plus [], like "n[]": this way, n will use the random sequence it has already haven as a sequence of value. For example, if you use "n = [1， 10]" in a loop of 10 times, then after the loop, n has 10 random values as a sequence inside it, denoted as nv[10]. In the next loop of 15 times, if you use "n[]", for the ith time of the loop, n uses the ith value from the sequence inside it, nv[i] as the value. If i > 10，i is mod by 10, so the final value is "nv[i % 10]".

After the sender finishing sending, it will output the variable values used for this time in "output.txt". Below is an example of the output:  

<html>
	
	seed 1571594936
	repeat [143]
		size 1000
		port 55556
	end
	repeat 10
		repeat 1	
			size 1024
			port 55555
		end
		repeat [1 2 2 1 2 2 3 4 1 3]	
			size 1024
			port 55556
		end
	end
	repeat 15
		repeat [3 3 3 3 3 3 3 3 3 3 3 3 3 3 3]	
			size 1024
			port 55555
		end
		repeat [1 2 2 1 2 2 3 4 1 3 1 2 2 1 2]	
			size 1024
			port 55556
		end
	end
	
</html>

As you can see, it outputs the seed, and if a variable is using a random value, it also outputs the sequence of the values of the variable. In this way, you can use the same seed to get the same random sequence.
