package main

import (
	"flag"
	"fmt"
	"net"
	"sync"
	"time"
)

func request(wg *sync.WaitGroup) {
	defer wg.Done()

	strEcho := "hello"
	reply := make([]byte, 256)

	conn, err := net.DialTimeout("tcp", "localhost:8080", 6*time.Second)
	if err != nil {
		fmt.Printf("Dial error: %s\n", err)
		return
	}

	defer conn.Close()

	_, err = conn.Write([]byte(strEcho))
	if err != nil {
		println("error request")
		return
	}

	_, err = conn.Read(reply)
	if err != nil {
		println("error request")
		return
	}

	println("done request")
}

func main() {
	var cnt int
	var wg sync.WaitGroup

	flag.IntVar(&cnt, "n", 10, "count of request")
	flag.Parse()

	for i := 1; i <= cnt; i++ {
		wg.Add(1)
		go request(&wg)
	}

	wg.Wait()
	println("Group done")
}
