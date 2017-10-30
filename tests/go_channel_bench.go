package main

import (
	"log"
	"sync"
	"time"
)

const COUNT = 100000000

var wg sync.WaitGroup

func main() {
	ch := make(chan int, 16)

	t1 := time.Now()

	go producer(ch, COUNT/2)
	go producer(ch, COUNT/2)

	wg.Add(1)
	consumer(ch, COUNT/2)
	wg.Add(1)
	consumer(ch, COUNT/2)

	wg.Wait()
	t2 := time.Now()
	log.Printf("ns diff: %v. len(ch): %d", t2.Sub(t1), len(ch))
}

func producer(ch chan int, count int) {
	for i := 0; i < count; i++ {
		ch <- i
	}
}

func consumer(ch chan int, count int) {
	for i := 0; i < count; i++ {
		<-ch
	}
	wg.Done()
}

/*
chuqq@chuqq-hp ~/t/a/tests> time ./go_channel_bench
2017/10/30 11:54:56 ns diff: 12.665532309s. len(ch): 0
16.39user 1.24system 0:12.66elapsed 139%CPU (0avgtext+0avgdata 1816maxresident)k
0inputs+0outputs (0major+212minor)pagefaults 0swaps
*/

