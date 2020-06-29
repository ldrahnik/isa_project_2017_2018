ASSESSMENT 
==========

20/20b (no assessment report)

Měření ztrátovosti a RTT
========================

Program monitoruje zadané síťové uzly. Pokud je paket prohlášený za ztracený nebo je překročená RTT hodnota pro daný uzel, je informace vypsána na standartní výstup. Pokud není vyžádán UDP protokol, využívají se ICMP zprávy echo request / reply. Pokud je UDP protokol vyžádaný přepínačem `-u` (je nutné uvést i port přepínačem `-p`), program zasílá náhodná data o velikosti 64B (lze změnit přepínačem `-s`), které poté kontroluje zda jsou přijata bez chyby. Pokud je uvedený UDP port přepínačem `-l`, program na něm naslouchá a všechny příchozí pakety odesílá zpět odesílateli s totožným obsahem. Program kontroluje všechny zadané síťové uzly paralelně. Pokud není uveden přepínač `-r`, je testována pouze ztrátovost paketů, ne RTT. RTT je v tomto případě zahrnuto pouze do celkové statistiky.

## Příklad spuštění:

```
./testovac -h
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

```
sudo ./testovac localhost -v
2020-06-29 17:11:34.24 104 bytes from localhost (127.0.0.1) time=0.126 ms
2020-06-29 17:11:34.34 104 bytes from localhost (127.0.0.1) time=0.035 ms
2020-06-29 17:11:34.44 104 bytes from localhost (127.0.0.1) time=0.036 ms
2020-06-29 17:11:34.54 104 bytes from localhost (127.0.0.1) time=0.033 ms
2020-06-29 17:11:34.64 104 bytes from localhost (127.0.0.1) time=0.032 ms
2020-06-29 17:11:34.74 104 bytes from localhost (127.0.0.1) time=0.032 ms
2020-06-29 17:11:34.84 104 bytes from localhost (127.0.0.1) time=0.032 ms
2020-06-29 17:11:34.94 104 bytes from localhost (127.0.0.1) time=0.031 ms
2020-06-29 17:11:35.04 104 bytes from localhost (127.0.0.1) time=0.138 ms
```

## Omezení programu:

## Rozšíření programu:

Vytvořená man stránka programu `dns.1`.

Vytvořený `dns.spec` soubor pro RPM balíček.

Vytvořený příkaz `make install`.

## Testování programu:

## Odevzdané soubory:

```
xdrahn00
├── LICENSE
├── Makefile
├── manual.pdf
├── src
│   ├── testovac.cpp
│   ├── testovac-error.h
│   ├── testovac.h
│   ├── testovac-params.cpp
│   └── testovac-params.h
├── testovac.1
└── testovac.spec

1 directory, 10 files
```
