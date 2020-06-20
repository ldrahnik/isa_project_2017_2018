ASSESSMENT 
==========

20/20b (no assessment report)

Měření ztrátovosti a RTT
============

## Příklad spuštění:

```
Testovac

monitors entered site nodes. Print informations on standard output when is
packet loss or excedation Round trip time (RTT) over specified value. When is not
specificated protocol and port with options -u and -p is used ICMP echo request/reply.
When is selected protocol UDP packet send random data with size 64B and expect the same
packet with the same content. When is specified port with option -l program listen on
UDP port and on received packets reply the same packets with the same content. Program
handle each of node parallely. When is not used option -r is tested only packets loss but
no RTT. RTT is in this case included only to the summary statistics.

Example of usage:

./testovac [-h] [-u] [-t <interval>] [-i <interval>] [-p <port>] [-l <port>] [-s <size>] [-r <value>] <node1> <node2> <node3> ... 

Options:
-h  -- show help message
-u  - select UDP protocol
-s - size of data for sending (default value is 56B)
-t <interval> -- interval in seconds for which is packets loss evaluated (default value is 300s)
-i <interval> -- interval in ms how often send testing messages (default value is 100ms)
-w <timeout> -- how long is program waiting but only when is response not received (default value is 2s)
-p <port> -- specification of UDP port
-l <port> -- specification of listening UDP port
-r <value> -- RTT treshold - reports only when is value exceeded
-v - verbose mode - program prints on stdout send packets
<node> or <node;RTT>-- IPv4/IPv6/hostname address of node, RTT which has priority for each node over global RTT -r <value>
```

## Omezení programu:

## Rozšíření programu:

Vytvořená man stránka programu `dns.1`.

Vytvořený `dns.spec` soubor pro RPM balíček.

Vytvořený příkaz `make install`.

## Testování programu:

## Odevzdané soubory:
