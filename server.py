import paho.mqtt.client as mqtt
import mqtt_handler

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker!")
        client.subscribe("your_topic")  # Replace with your MQTT topic

def on_message(client, userdata, msg):
    mqtt_handler.handle_message(msg.topic, msg.payload)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("your_broker_address", 1883)  # Replace with your broker address and port
client.loop_forever()
