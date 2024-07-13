# ftp-app

An implementation of a websocket server and client.
The server listens for incoming connections from the client(s), and can:

- Listen for write requests, the client sends a file, and the server writes it in the specified directory.
- Listen for read requests, basically the opposite of a write requests, where the server sends the file and the clients receives it.
- Listen for list requests, the server sends data regarding the specified path from the client, like an ls -la command.

This was made as a part of the Operative Systems II computer science course at Sapienza Università di Roma.


## Installation

Clone the repo
```bash
git clone https://github.com/saverioscagnoli/ftp-app
cd ftp-app
```
compile the code, I use gcc.

Server:
```bash
gcc -o ./bin/server.out src/server/main.c src/server/utils.c
```
Client:
```bash
gcc -o ./bin/client.out src/client/main.c src/client/utils.c
```

## Usage

To start the server:
```bash
./bin/server.out -a <ip_address> -p <port> -d <root_dir>
```

This will start a websocket server on the -a address:port and the dir where the files are stored, and it creates it if not present.

To use the client:
```bash
./bin/client.out -a <ip_address> -p <port> (-w / -l / -r) -f <f_path> -o <o_path> (only for -w and -r)
```

If it is a write request, the client will open specified with f_path, and send it to the server, where it will write it on <root_dir>/<o_path>

if o_path is not specified, it will use f_path for both.

If it is a read request, the client will request the f_path file on the server, and write it locally on the o_path file.

if o_path is not specified, it will use f_path for both.

At last, if it is a list request, the client will request a ls -la command on the f_path directory.

## License
MIT License 2024 Saverio Scagnoli