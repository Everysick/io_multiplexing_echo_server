package main

import (
	"flag"
	"fmt"
	"net"
	"sync"
	"time"
)

var success int
var failure int
var count int

func request(mul int, wg *sync.WaitGroup) {
	defer func() { count++ }()
	defer wg.Done()

	strHello := []byte("Hello")
	strBye := []byte("Bye!!")

	reply := make([]byte, 256)

	conn, err := net.DialTimeout("tcp", "localhost:8080", 6*time.Second)
	if err != nil {
		failure++
		return
	}

	defer conn.Close()

	for i := 0; i < mul; i++ {
		_, err = conn.Write(strHello)
		if err != nil {
			failure++
			return
		}

		_, err = conn.Read(reply)
		if err != nil {
			failure++
			return
		}

	}

	_, err = conn.Write(strBye)
	if err != nil {
		failure++
		return
	}

	success++
}

func main() {
	var cnt int
	var mul int

	var wg sync.WaitGroup

	count = 0
	success = 0
	failure = 0

	flag.IntVar(&cnt, "n", 10, "Number of client")
	flag.IntVar(&mul, "m", 10, "Number of echo request per client")
	flag.Parse()

	wg.Add(cnt)

	for i := 0; i < cnt; i++ {
		go request(mul, &wg)
	}

	wg.Wait()

	fmt.Printf("All: %d, Success: %d, Failure: %d \n", count, success, failure)
}
