package main 
import (
	"golang.org/x/net/websocket"
	"fmt"
	"log"
	"net/http"
	"github.com/tarm/serial"
	"time"
	"strconv"
	"sync"
)

/**Define global variables
*/
var i = 0
var j = 0
var currentTime int64
var previousTime int64 
var currentShortTime int64
var previousShortTime int64 
var period int64
var shortPeriod int64
var serverDataSent bool
var clientDataRecieved bool
var clientConfirm bool
var timerStarted bool

/**Sending Function to send data over Serial
*/
func send(data string) {
	c := &serial.Config{Name: "/dev/ttyUSB0", Baud: 115200}
	s, err := serial.OpenPort(c)
        if err != nil {
                log.Fatal(err)
        }
    _, err1 := s.Write([]byte(data))
        if err1 != nil {
                log.Fatal(err1)
        }
        s.Flush();
        fmt.Println("Data Sent Successfully")
}

/**Function called when the server starts to listen for incoming
* connection(s) and handles them by sending and receving messages
*/
func websocket_send (ws *websocket.Conn) {
	var wg sync.WaitGroup
    wg.Add(2)
    var command string
    currentTime = int64(time.Now().Unix())
    previousTime = int64(time.Now().Unix())

    /**Receving Thread:
    * Actively looks for new connections and is utilized first 
    * when the Arduino client sends its first message after a 
    * successful handshake
    */
	go func () {
		for {
			var temp string
			err := websocket.Message.Receive(ws, &temp)

			/**Handles Errors in Receiving Messages
			*/

			if err != nil {
				fmt.Println("Error:", err)
				continue
			}

			/**Handles length of the message to clear out unecessary
			* bits at the end
			*/
			if (len(temp) > 4) {
				fmt.Println("Message Received from Arduino", temp[:len(temp)-3])
			} else {
				fmt.Println("Message Received from Arduino", temp)
			}

			/**After reading client Arduino's messages, decides the appropriate reply,
			* if one is needed to be sent for it. Otherwise, prepares its own message
			* which has been implemented below in the Sending Thread.
			*/
			if (temp[:1] == "1") {
					command ="Red Light On Now! qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwerty\n";
				} else if (temp[:1] == "2") {
					command ="Green Light On Now! qwertyuiopqwertyuiopqwertyuiopqwertyuiopqwer\n";
				} else if (temp[0:3] == "ACK") {
					fmt.Println(temp)
					no,_ := strconv.Atoi(temp[4:len(temp)-1]);
					fmt.Println("ACK " + strconv.Itoa(no) + " Received from client");
					clientConfirm = true;
				} else if (temp == "PTI ALERT CLIENT\n") {
					j++
					ACK_Send:="ACK " + strconv.Itoa(j)
					err3:=websocket.Message.Send(ws,ACK_Send)
					if (err3!=nil) {
						fmt.Println("Error:", err3)
					}
					clientDataRecieved = true;
				}


			if ((clientDataRecieved || clientConfirm) && !timerStarted) {
				previousShortTime = int64(time.Now().Unix())
				timerStarted = true
			}

			currentShortTime = int64(time.Now().Unix())
			if (clientConfirm && clientDataRecieved) {
				period+=10
				clientConfirm = false
				clientDataRecieved = false
				serverDataSent = false
				timerStarted = false
				fmt.Println("Adding")

			} else if ((currentShortTime - previousShortTime) > shortPeriod) {

				/** Make sure late replies are avoided
				That is if a late reply is sent then it should be ignored insead of going into the loop
				*/
				//period/=2
				if (period >= 20) {
					period/=2;			
				} else if (period < 10) {
					period = 10;			
				}
				clientConfirm = false
				clientDataRecieved = false
				serverDataSent = false
				timerStarted = false
				fmt.Println("Backing off")
			}
		}
	}()

	/** Sending Thread:
	* Handles the sending of ACKs, Periodic Alerts, hardware triggers 
	* and general information
	*/
	go func () {
		for {
			fmt.Println("Sending message to client")
			fmt.Println("Iteration No.: ", i)
			var command1 string;

			/** Maintains message size to be sent
			*/

			if (len(command) > 64)  {
				command1 = command[0:64];
			} else {
				command1 = command;
			}

			/** Decides the messages to be sent, the inter-message time
			* and sends them.
			*/

			fmt.Println(command1)
			if (i < 1000) {
				//send(command[:1])
				err1 := websocket.Message.Send(ws, command1) 
				if err1 != nil {
					fmt.Println("Error:", err1)
					}
				fmt.Print("\n");
				duration := 2*time.Second
 				time.Sleep(duration)
 				i++
  			}

  			/** Sends Periodic Alerts
  			*/

 			currentTime = int64(time.Now().Unix())
  			if ((currentTime - previousTime) > period) {
  				command1 =  "PTI ALERT SERVER"
  				serverDataSent = true;
  				previousTime = currentTime
  				err2 := websocket.Message.Send(ws, command1) 	
  				if err2 != nil {
					fmt.Println("Error:", err2)
				}
  			}
		}
	}()

	/** Waits for thread completion
	*/

	wg.Wait()
}

func main () {

	/** Sets basic variables and runs the port on the desried port
	*/
	serverDataSent = false
	clientConfirm = false
	clientDataRecieved = false
	timerStarted = false
	period = 10
	fmt.Println("Latency Checker...")
	http.Handle("/echo", websocket.Handler(websocket_send))
	currentTime = int64(time.Now().Unix())
	previousTime = int64(time.Now().Unix())
	currentShortTime = int64(time.Now().Unix())
	previousShortTime = int64(time.Now().Unix())
	err:= http.ListenAndServe(":1235", nil)
	if err != nil {
		log.Fatal(err)
	}
}
