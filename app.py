import csv
import threading
from datetime import datetime

import paho.mqtt.client as mqtt

slot_data = {
    "temperature": None,
    "humidity": None,
    "light": None,
}
csv_filename = "data/sensor_data.csv"
lock = threading.Lock()


def format_timestamp(dt):
    return dt.strftime("%Y-%m-%d %H:%M:%S") + ",000"


def save_to_csv(filename, timestamp_str, data):
    with open(filename, 'a', newline='') as csv_file:
        fieldnames = ["timestamp", "temperature", "humidity", "light"]
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)

        if csv_file.tell() == 0:
            writer.writeheader()

        row = {
            "timestamp": timestamp_str,
            "temperature": data["temperature"],
            "humidity": data["humidity"],
            "light": data["light"],
        }

        writer.writerow(row)
        csv_file.flush()


def periodic_slot_writer():
    while True:
        now = datetime.now()
        # 슬롯의 시작 시각 = 현재 시간 기준으로 10초 배수 시점
        seconds = (now.second // 10) * 10
        slot_time = now.replace(second=seconds, microsecond=0)

        with lock:
            timestamp_str = format_timestamp(slot_time)
            save_to_csv(csv_filename, timestamp_str, slot_data.copy())
            print("Saved slot:", timestamp_str, slot_data)

            # 다음 슬롯을 위해 초기화
            slot_data["temperature"] = None
            slot_data["humidity"] = None
            slot_data["light"] = None

        # 다음 10초 슬롯까지 대기
        now = datetime.now()
        sleep_time = 10 - (now.timestamp() % 10)
        threading.Event().wait(sleep_time)


# MQTT callback function
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected OK")
        client.subscribe("yonsei/gsi/smart-desk/temperature")
        client.subscribe("yonsei/gsi/smart-desk/humidity")
        client.subscribe("yonsei/gsi/smart-desk/light")
    else:
        print("Bad connection. Returned code=", rc)


def on_disconnect(client, userdata, rc=0):
    print("Disconnected. Code:", rc)


def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode("utf-8")
    print(f"[{topic}] {payload}")

    with lock:
        if topic.endswith("temperature"):
            slot_data["temperature"] = float(payload)
        elif topic.endswith("humidity"):
            slot_data["humidity"] = float(payload)
        elif topic.endswith("light"):
            slot_data["light"] = int(payload)


# MQTT client setting
client = mqtt.Client()
client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message
client.connect("broker.emqx.io", 1883)

# 10초 단위 기록
writer_thread = threading.Thread(target=periodic_slot_writer, daemon=True)
writer_thread.start()

# MQTT loop start
client.loop_forever()
