package main

import (
	"fmt"
	"log"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/tarm/serial"

	"github.com/go-telegram-bot-api/telegram-bot-api"
)

func main() {
	token := os.Getenv("TELEGRAM_TOKEN")
	channel, err := strconv.ParseInt(os.Getenv("CHANNEL_ID"), 10, 64)
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

	ch, errCh := PingPlatform()

	go func(errCh <-chan error) {
		for err := range errCh {
			if err != nil {
				fmt.Println(err)
			}
		}
	}(errCh)

	botReceivedAnswerFromUser := false
	go func(ch <-chan bool) {
		const fiveMinutes = 60 * 5
		count := 0
		for pressed := range ch {
			if pressed {
				count = 0
				continue
			}
			count++
			if count >= fiveMinutes && !botReceivedAnswerFromUser {
				msg := tgbotapi.NewMessage(int64(channel), "На платформе нет воды. Засыхаем!")
				bot.Send(msg)
			}
		}
	}(ch)

	updates, err := bot.GetUpdatesChan(u)

	for update := range updates {
		if update.Message == nil {
			continue
		}

		if strings.Contains(update.Message.Text, "@"+bot.Self.UserName) && strings.Contains(update.Message.Text, "заказали") && strings.Contains(update.Message.Text, "воду") && strings.Contains(update.Message.Text, "брат") {
			if !botReceivedAnswerFromUser {
				go func() {
					time.Sleep(time.Hour * 24)
					botReceivedAnswerFromUser = false
				}()
			}
			botReceivedAnswerFromUser = true
			msg := tgbotapi.NewMessage(update.Message.Chat.ID, "понял тебя, брат!")
			msg.ReplyToMessageID = update.Message.MessageID
			bot.Send(msg)
		}
	}
}

//PingPlatform pings platform to get current water quantity, if unable to reach the platform, sends error
func PingPlatform() (<-chan bool, <-chan error) {

	const chkCmd = 0xa6 //see hw part
	buf := make([]byte, 8)
	scfg := &serial.Config{Name: "/dev/rfcomm0", Baud: 9600}
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
