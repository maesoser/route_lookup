#  Two-level software multi-bit trie route lookup algorithm

This work is based on the paper from Pankaj Gupta, Steven Lin, and Nick McKeown called "[Routing Lookups in Hardware at Memory Access Speeds](http://tiny-tera.stanford.edu/~nickm/papers/Infocom98_lookup.pdf)".

##### Compile using:

`make `

##### Check it using:
`valgrind --tool=memcheck --leak-check=yes ./route_lookup RIB prueba`

## Description of the implemented algorithm.

The code is based on two main lists:

- **mtable:** Stores 2^24 interfaces each of them is sixteen bits long. The index of this table is made from the 24 MSB of and Ip Address. For instance, an entry like this:
 
		IP = 10.10.5.3 -> IF = 3

In the table would be stored like:

		mtable[657925] = 3
	
If the first bit on the interface field is 1, it means it represents an index to a secondary table (`stable`) which stores the special networks (with higher masks) that are specified in the least significant bits. If not, it is just the interface ID.

- **stable:** Stores the networks whose netmask is higher than 24.

####  There are basically two main methods which fill the two tables and perform the route lookup.

- `initializeFIB():`

This is the most complex and large method of this code. It reads the route table file and, depending of the content of the line, make one thing or another.

**If the mask is less than 24:** It stores the interface written in the file in the position defined by the last 24 bits of the IP.

**If the mask is equal or more than 24:** It reads the memory stored in the main table. If it has a 1 in the "special bit", indicating it's an index to `stable` instead of an interface, the method goes to stable and writes the interface in the corresponding address inside the secondary table.

If it has not a 1, meaning that is the first route entry that extends this this IP range more than 24 bits, it resizes the second table in order to store 256 new positions corresponding to the last byte of an IP, 192.123.23.X . After that, it copies the interface stored in the main table to the second table and after that it updates the information stored in the main table by writting a 1 in the 16th bit and the index to the stable entry in the rest of the bits.

- `interface_lookup(uint32_t *IP_lookup, short int *ntables,unsigned short *interface):`

It looks for an IP inside the route lookup tables stored in RAM. To do that it uses the first 24 bits of the IP as an index to the main table.

If the 16th bit is 0 it means that the data of mtable is the interface. It returns the interface.
If the 16th bit is 1 it means that the data of mtable is the index to stable. It returns the content of stable.

## Under which circumstances this algorithm performs better than the linear search algorithm?

In any circumstance because linear Search is an algorithm that always make more than one access to DRAM in order to return an interface. Nevertheless our algorithm beats the Linear Search algorithm even more when the routing table is a complex one (as we can see in the second simulation performed on a Pentium M) because in these situations Linear Search has to perform more access to memory while our multibit-Trie algorithm continues performing the same table accesses (1-2)

The Two-level software multi-bit trie that we implemented make 2 access to the memory in the worst case so the performance of our algorithm is only limited by the speed of our RAM. We have to take into account that our algorithm runs over an Operative System. That creates some "noise" in our results because our machine is running more processes besides our code. Implementing this algorithm over a dedicated hardware configuration would have improve this results even more.

| COMBINATION | Multibit1 | Multibit2 | Linear  |
|-------------|-----------|-----------|---------|
| t0 with p0  | 0.62      | 0.12      |  2.88   |
| t0 with p1  | 0.36      | 0.38      |  4.34   |
| t0 with p2  | 0.18      | 0.58      |  6.12   |
| t1 with p0  | 0.25      | 0.25      |  1.00   |
| t1 with p1  | 0.34      | 0.08      |  1.06   |
| t1 with p2  | 0.58      | 0.48      |  0.86   |
| t2 with p0  | 0.50      | 0.12      |  12.75  |
| t2 with p1  | 0.25      | 0.40      |  18.80  |
| t2 with p2  | 0.06      | 0.48      |  14.88  |	Executed on monitor01.lab.it.uc3m.es

| COMBINATION | Multibit | Linear  |
|-------------|----------|---------|
| t with p0   | 1.33     |  34.00  |
| t with p1   | 1.12     |  25.96  |
| t with p2   | 1.14     |  32.16  |
| t with p3   | 1.18     |  28.70  |
| ts with p0  | 1.33     |  3.67   |
| ts with p1  | 1.28     |  1.68   |
| ts with p2  | 1.24     |  1.74   |
| ts with p3  | 1.36     |  1.80   | Executed on a Debian 7.8 Pentium M @ 1400MHz

## Argue if this algorithm is scalable for IPv6 (128 bit addresses, where 64 bits are the network prefix).

Although at first sight this algorithm seems to be scalable to IPv6 with just some minor changes in the code, it would be very difficult to scale it due to memory restrictions. Now we are using 35 Mbytes roughly speaking (our main table has 2^24 entries, each of them occupy 2 bytes, that is 33.5 Mbytes). With IpV6 the main table (which is not dynamically allocated) would occupy 2^64 x 2 bytes wich is 3.51x10^13 Mbytes, 35 Exabytes).

Some improvement should be deployed in order to decrease the memory usage. Using an intermediate length table ,a multiple table scheme or a two-phase inter-node compression algorithm (as Michel Hanna, Sangyeun Cho, and Rami Melhem explain on its paper "[A Novel Scalable IPv6 Lookup Scheme Using Compressed Pipelined Tries](http://dl.acm.org/citation.cfm?id=2008820)") would be necessary to adapt this algorithm to IPv6 in a real scenario.

Another interesting research path could take us to explore space-efficient probabilistic data structures like [cuckoo](http://dl.acm.org/citation.cfm?id=2674994) or [bloom filters](http://dl.acm.org/citation.cfm?id=1080114).

So the escalation of this algorithm, although it is possible, would imply some deep changes that will change substantially the structure of the code and would suppose to leave the main concept of the algorithm proposed by Gupta, which is the use of prefix expansion to simplify the ip-lookup process due to continued decreasing cost of DRAM memories.

## Bibliography

 * 	[Understanding IP Addresses and binary](http://www.watchguard.com/infocenter/editorial/135183.asp)

 * 	[C Pointers](http://www.eskimo.com/~scs/cclass/notes/sx10b.html)

 * 	[Bitwise Operators in C](http://www.cprogramming.com/tutorial/bitwise_operators.html)

 * 	[Valgrind User Manual](http://valgrind.org/docs/manual/manual.html)

 * [Routing Lookups in Hardware at Memory Access Speeds](http://tiny-tera.stanford.edu/~nickm/papers/Infocom98_lookup.pdf)

 * [Wikipedia article for IPv6](http://es.wikipedia.org/wiki/IPv6)

 * [A Novel Scalable IPv6 Lookup Scheme Using Compressed Pipelined Tries](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.221.1109&rep=rep1&type=pdf)

 * [FlashTrie: Hash-based Prefix-Compressed Trie for IP Route Lookup Beyond 100Gbps](http://eeweb.poly.edu/chao/docs/public/fthpctirlb100g.pdf)

 * [Projecting IPv6 Packet Forwarding Characteristics Under Internet-wide Deployment](http://conferences.sigcomm.org/sigcomm/2007/ipv6/1569042943.pdf)
