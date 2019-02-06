package main

import (
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/go-telegram-bot-api/telegram-bot-api"
	"github.com/tarm/serial"
)

const (
	envToken  = "TELEGRAM_TOKEN"
	envChid   = "CHANNEL_ID"
	envSerial = "SERIAL_PORT"

	defaultSerial = "/dev/rfcomm0"
	defaultToken  = "HUYOKEN"
	defaultChid   = 0
)

func main() {
	token := os.Getenv(envToken)
	if token == "" {
		token = defaultToken
	}
	serialPort := os.Getenv(envSerial)
	if serialPort == "" {
		serialPort = defaultSerial
	}

	channel, err := strconv.ParseInt(os.Getenv(envChid), 10, 64)
	if err != nil {
		log.Panic(err)
	}
	bot, err := tgbotapi.NewBotAPI(token)
	if err != nil {
		log.Panic(err)
	}

	bot.Debug = true

	log.Printf("Authorized on account %s", bot.Self.UserName)

	u := tgbotapi.NewUpdate(0)
	u.Timeout = 60

	ch, errCh := PingPlatform(serialPort)

	go func(errCh <-chan error) {
		for err := range errCh {
			if err != nil {
				fmt.Printf("Something went wrong: %v", err)
			}
		}
	}(errCh)

	botReceivedAnswerFromUser := false
	go func(ch <-chan bool) {
		const timeoutSec = 60 * 5
		count := 0
		for pressed := range ch {
			fmt.Printf("%d: Platform pressed : %v\n", count, pressed)
			if pressed {
				fmt.Println("Something on platform")
				if count >= timeoutSec {
					msg := tgbotapi.NewMessage(int64(channel), "Что-то поставили на платформу. Может воду?")
					bot.Send(msg)
				}
				count = 0
				continue
			}
			count++
			if count >= timeoutSec && !botReceivedAnswerFromUser {
				msg := tgbotapi.NewMessage(int64(channel), "На платформе нет воды. Засыхаем!")
				bot.Send(msg)
				time.Sleep(time.Minute * 1)
			}
		}
	}(ch)

	updates, err := bot.GetUpdatesChan(u)

	for update := range updates {
		if update.Message == nil {
			continue
		}

		if strings.Contains(update.Message.Text, "@"+bot.Self.UserName) && strings.Contains(update.Message.Text, "заказали") /*&& strings.Contains(update.Message.Text, "воду") && strings.Contains(update.Message.Text, "брат")*/ {
			if !botReceivedAnswerFromUser {
				go func() {
					sleepDuration := time.Hour * 24
					time.Sleep(sleepDuration)
					botReceivedAnswerFromUser = false
				}()
			}
			botReceivedAnswerFromUser = true
			msg := tgbotapi.NewMessage(update.Message.Chat.ID, "Принято! Жду 24 часа")
			msg.ReplyToMessageID = update.Message.MessageID
			bot.Send(msg)
		}
	}
}

//PingPlatform pings platform to get current water quantity, if unable to reach the platform, sends error
func PingPlatform(serialPort string) (<-chan bool, <-chan error) {

	const chkCmd = 0xa6 //see hw part
	buf := make([]byte, 8)
	scfg := &serial.Config{Name: serialPort, Baud: 9600}
	chData := make(chan bool)
	chError := make(chan error)

	//returns true if there is water bottle on platform. false otherwise
	pfCheckPressed := func(buf []byte) (bool, error) {
		sp, err := serial.OpenPort(scfg)
		if err != nil {
			return false, err
		}
		defer sp.Close()
		n, err := sp.Write([]byte{chkCmd})
		if err != nil {
			return false, err
		}
		n, err = sp.Read(buf)
		if err != nil {
			return false, err
		}
		if n != 1 || (buf[0] != 0x00 && buf[0] != 0x20) {
			return false, fmt.Errorf("Wrong data from platform : %v", buf[:n])
		}
		return buf[0] == 0x00, nil
	}

	go func() {
		for {
			time.Sleep(1 * time.Second)
			pressed, err := pfCheckPressed(buf)
			if err != nil {
				chError <- err
				continue
			}
			chData <- pressed
		}
	}()

	return chData, chError
}
