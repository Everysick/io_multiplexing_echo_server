package main

import (
	"flag"
	"fmt"
	"net"
	"strconv"
	"sync"
	"time"
)

var success int
var failure int
var count int

func request(num int, wg *sync.WaitGroup) {
	defer func() { count++ }()
	defer wg.Done()

	strEcho := []byte("hello" + strconv.Itoa(num))
	reply := make([]byte, len(strEcho))

	conn, err := net.DialTimeout("tcp", "localhost:8080", 6*time.Second)
	if err != nil {
		failure++
		return
	}

	defer conn.Close()

	_, err = conn.Write(strEcho)
	if err != nil {
		failure++
		return
	}

	time.Sleep(250 * time.Millisecond)

	_, err = conn.Read(reply)
	if err != nil || string(strEcho) != string(reply) {
		failure++
		return
	}

	time.Sleep(250 * time.Millisecond)

	_, err = conn.Write(strEcho)
	if err != nil {
		failure++
		return
	}

	success++
}

func main() {
	var cnt int
	var wg sync.WaitGroup

	count = 0
	success = 0
	failure = 0

	flag.IntVar(&cnt, "n", 10, "count of request")
	flag.Parse()

	wg.Add(cnt)

	for i := 0; i < cnt; i++ {
		go request(i, &wg)
	}

	wg.Wait()

	fmt.Printf("All: %d, Success: %d, Failure: %d \n", count, success, failure)
}
