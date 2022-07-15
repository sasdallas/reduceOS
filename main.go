package main
import(
	"time"
	"fmt"
)

func main(){
	ticker := time.NewTicker(time.Second*30)
	done:= make(chan bool)

	go func(){
		for {
			select {
				case <-done:
					return
				case t:=<-ticker.C:
					fmt.Println("Ticket at ",t)
			}
		}
	}()
	time.Sleep(time.Hour*1)
	ticker.Stop()
	done<-true
	fmt.Println("ticker ssop afer 1 hour")
}
