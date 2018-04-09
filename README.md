# I/O Multiplexing echo servers

Example implementations of IO multiplexed echo server over TCP/IP.

## Contents

|                       | model          | worker   | io multiplex | stable |
|-----------------------|:--------------:|:--------:|:------------:|:------:|
| simple echo           | single-process | 1        | x            | o      |
| thread echo           | multi-thread   | variable | x            | x      |
| fork echo             | multi-process  | variable | x            | x      |
| pre-thread echo       | multi-thread   | variable | x            | o      |
| pre-fork echo         | multi-process  | variable | x            | o      |
| simple epoll echo     | single-process | 1        | o            | o      |
| pre-thread epoll echo | multi-thread   | variable | o            | o      |
| pre-fork epoll echo   | multi-process  | variable | o            | o      |

## Testing

- Run `make`

- Launch any server

- Run `client.go` with options

```sh
$ time go run client.go -n xxx -m xxx
    -m int
      Number of echo request per client (default 10)
    -n int
      Number of client (default 10)
```

## Author

takamasa saichi
