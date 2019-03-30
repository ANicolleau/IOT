#include "mbed.h"
#include "zest-radio-atzbrf233.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

// Network interface
NetworkInterface *net;

namespace {
#define PERIOD_MS 12000
}


int arrivedcount = 0;
const char* feedsMessage = "anicolleau/feeds/message";
const char* feedsTemperature = "anicolleau/feeds/temprature";
const char* feedLed = "anicolleau/feeds/lumiere";
const char* feedHum = "anicolleau/feeds/humide";

static DigitalOut led1(LED1);
I2C i2c(I2C1_SDA, I2C1_SCL);
AnalogIn capt_hum(ADC_IN1);
uint8_t lm75_adress = 0x48 << 1;

/* Printf the message received and its configuration */
void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

void methode(){
		char cmd[2];
		cmd[0] = 0x00; // adresse registre temperature

		i2c.write(lm75_adress, cmd, 1);
		i2c.read(lm75_adress, cmd, 2);
		float tension_temp = capt_hum.read();
		//float measure_percent = (tension_temp - air value/*prendre l'humidité de l'air*/) * 100.0 / (tension_temp - 0);
		// vue que mon water value est de 1 et le air_value de 0
		float measure_percent = tension_temp * 100;
		float temperature = ((cmd[0] << 8 | cmd[1] ) >> 7) * 0.5;

	// Build the socket that will be used for MQTT
	    MQTTNetwork mqttNetwork(net);

	    // Declare a MQTT Client
	    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

	// Connect the socket to the MQTT Broker
	    const char* hostname = "io.adafruit.com";
	    uint16_t port = 1883;
	    printf("Connecting to %s:%d\r\n", hostname, port);
	    int rc = mqttNetwork.connect(hostname, port);
	    if (rc != 0)
	        printf("rc from TCP connect is %d\r\n", rc);
		// Connect the MQTT Client
		MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
		data.MQTTVersion = 3;
		data.clientID.cstring = "test";
		data.username.cstring = "anicolleau";
		data.password.cstring = "213f7685741d4b268fdd28fd070a58f6";


		if ((rc = client.connect(data)) != 0)
			printf("rc from MQTT connect is %d\r\n", rc);

		// Subscribe to the same topic we will publish in
		if ((rc = client.subscribe(feedsMessage, MQTT::QOS2, messageArrived)) != 0)
			printf("rc from MQTT subscribe is %d\r\n", rc);

		if ((rc = client.subscribe(feedsTemperature, MQTT::QOS2, messageArrived)) != 0)
			   printf("rc from MQTT subscribe is %d\r\n", rc);

		if ((rc = client.subscribe(feedLed, MQTT::QOS2, messageArrived)) != 0)
				printf("rc from MQTT subscribe is %d\r\n", rc);

		if ((rc = client.subscribe(feedHum, MQTT::QOS2, messageArrived)) != 0)
				printf("rc from MQTT subscribe is %d\r\n", rc);

		// Send a message with QoS 0
		MQTT::Message message;
		MQTT::Message temp;
		MQTT::Message humidite;
		MQTT::Message lumiere;

		// message
		char buf[100];
		sprintf(buf, "La température est de : %f", temperature);

		message.qos = MQTT::QOS0;
		message.retained = false;
		message.dup = false;
		message.payload = (void*)buf;
		message.payloadlen = strlen(buf)+1;

		rc = client.publish(feedsMessage, message);

		//température
		char buff[100];
		sprintf(buff, "%f", temperature);

		temp.qos = MQTT::QOS0;
		temp.retained = false;
		temp.dup = false;
		temp.payload = (void*)buff;
		temp.payloadlen = strlen(buff)+1;

		rc = client.publish(feedsTemperature, temp);

		//humidite
		char bufff[100];
		sprintf(bufff, "%f", measure_percent);

		humidite.qos = MQTT::QOS0;
		humidite.retained = false;
		humidite.dup = false;
		humidite.payload = (void*)bufff;
		humidite.payloadlen = strlen(bufff)+1;

		rc = client.publish(feedHum, humidite);

		//lumiere
		/*char buffff[100];
		sprintf(buffff, "%f", led);

		lumiere.qos = MQTT::QOS0;
		lumiere.retained = false;
		lumiere.dup = false;
		lumiere.payload = (void*)buffff;
		lumiere.payloadlen = strlen(buffff)+1;

		rc = client.publish(feedLed, lumiere);*/
		printf("salut %f", measure_percent);

    }

// MQTT demo
int main() {
	int result;



    // Add the border router DNS to the DNS table
    nsapi_addr_t new_dns = {
        NSAPI_IPv6,
        { 0xfd, 0x9f, 0x59, 0x0a, 0xb1, 0x58, 0, 0, 0, 0, 0, 0, 0, 0, 0x00, 0x01 }
    };
    nsapi_dns_add_server(new_dns);

    printf("Starting MQTT demo\n");

    // Get default Network interface (6LowPAN)
    net = NetworkInterface::get_default_instance();
    if (!net) {
        printf("Error! No network inteface found.\n");
        return 0;
    }

    // Connect 6LowPAN interface
    result = net->connect();
    if (result != 0) {
        printf("Error! net->connect() returned: %d\n", result);
        return result;
    }






    // méthode était ici

		// yield function is used to refresh the connexion
		// Here we yield until we receive the message we sent
		// il était sur 1
		//while (true /*arrivedcount < 1*/)
    	//client.yield(100);
    	while(true){
			ThisThread::sleep_for(PERIOD_MS);
			methode();
    	}

		// Disconnect client and socket
		/*client.disconnect();
		mqttNetwork.disconnect();*/

		// Bring down the 6LowPAN interface
		net->disconnect();
		printf("Done\n");

}



