import paho.mqtt.client as mqtt
import mqtt_handler
import utils
import time
import random
import json

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker!")
        client.subscribe(mqtt_handler.topic_validate)

def on_message(client, userdata, msg):
    mqtt_handler.handle_message(msg.topic, msg.payload, client)

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    
    broker_address = "192.168.1.40"
    broker_port = 1883
    client.connect(broker_address, broker_port)
    
    client.loop_start()  # Start the loop in a separate thread

    try:
        while True:
            x, y,z = utils.get_coordinates_from_server()
            coordinates = {"x": x, "y": y,"id": random.randint(1, 10)}
            client.publish(mqtt_handler.topic_coordinate, json.dumps(coordinates))
            print(f"Sent coordinates: {coordinates}")
            time.sleep(3)  # Adjust delay between sending messages as needed
    except KeyboardInterrupt:
        print("Exiting...")
        client.disconnect()
        client.loop_stop()

if __name__ == "__main__":
    main()
