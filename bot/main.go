package main

import (
	"fmt"
	"log"
	"math/rand"
	"os"
	"strconv"
	"strings"
	"time"

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

	go func(ch <-chan int) {
		for wq := range ch {
			if wq < 1 {
				msg := tgbotapi.NewMessage(int64(channel), "Осталось меньше 1 литра!!!")
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
			msg := tgbotapi.NewMessage(update.Message.Chat.ID, "понял тебя, брат!")
			msg.ReplyToMessageID = update.Message.MessageID
			bot.Send(msg)
		}
	}
}

//PingPlatform pings platform to get current water quantity, if unable to reach the platform, sends error
func PingPlatform() (<-chan int, <-chan error) {

	ch := make(chan int, 1)
	errCh := make(chan error, 1)

	go func() {
		for {

			//dummy data code to check if bot is working
			random := rand.New(rand.NewSource(time.Now().UnixNano()))
			waterQuantity := random.Intn(20)
			fmt.Println("New Water Quantity: ", waterQuantity)
			ch <- waterQuantity
			errCh <- nil
			time.Sleep(1 * time.Second)
		}
	}()

	return ch, errCh
}
