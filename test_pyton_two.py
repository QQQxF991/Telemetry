#!/usr/bin/env python3
import socket
import struct
import time

def create_message_with_crc(device_id, value, timestamp):
    device_id_byte = bytes([device_id])
    value_bytes = struct.pack('>f', value)  
    timestamp_bytes = struct.pack('>Q', timestamp)  
    
    data = device_id_byte + value_bytes + timestamp_bytes
    
    crc = 0
    for byte in data:
        crc ^= byte
    
    return data + bytes([crc])

def send_test_messages():
    print("Подключение к бинарному серверу...")
    
    messages = [
        (1, 42.5, int(time.time())),
        (2, 18.7, int(time.time()) + 1),
        (3, -3.14, int(time.time()) + 2),
        (1, 99.9, int(time.time()) + 3),
    ]
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 9001))
        
        for device_id, value, timestamp in messages:
            message = create_message_with_crc(device_id, value, timestamp)
            
            print(f"Отправка: device={device_id}, value={value}, "
                  f"timestamp={timestamp}, crc={message[-1]}")
            print(f"  Длина сообщения: {len(message)} байт")
            
            sock.send(message)
            time.sleep(0.1)
        
        sock.close()
        print("\n✓ Сообщения отправлены")
        
    except Exception as e:
        print(f"✗ Ошибка: {e}")

def check_http_api():
    import subprocess
    import json
    
    print("\nПроверка HTTP API...")
    
    for device_id in [1, 2, 3]:
        print(f"\n--- Устройство {device_id} ---")
        
        result = subprocess.run(
            ['curl', '-s', f'http://localhost:8080/device/{device_id}/latest'],
            capture_output=True, text=True
        )
        
        if result.returncode == 0:
            print(f"GET /latest: {result.stdout.strip()}")
            
            try:
                data = json.loads(result.stdout)
                if "error" not in data:
                    print(f"  ✓ Устройство {device_id} имеет данные")
            except:
                pass
        
        result = subprocess.run(
            ['curl', '-s', f'http://localhost:8080/device/{device_id}/stats'],
            capture_output=True, text=True
        )
        
        if result.returncode == 0:
            print(f"GET /stats: {result.stdout.strip()}")

def debug_server():
    print("\nПрямой тест с созданием сообщения...")
    
    device_id = 1
    value = 123.456
    timestamp = int(time.time())
    
    device_id_byte = bytes([device_id])
    value_bytes = struct.pack('>f', value)
    timestamp_bytes = struct.pack('>Q', timestamp)
    
    data = device_id_byte + value_bytes + timestamp_bytes
    
    print(f"Данные (13 байт): {data.hex()}")
    print(f"  device_id: {device_id}")
    print(f"  value: {value} -> bytes: {value_bytes.hex()}")
    print(f"  timestamp: {timestamp} -> bytes: {timestamp_bytes.hex()}")
    
    crc = 0
    for byte in data:
        crc ^= byte
    
    message = data + bytes([crc])
    
    print(f"Полное сообщение (14 байт): {message.hex()}")
    print(f"CRC: {crc}")
    
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 9001))
        sock.send(message)
        sock.close()
        print("✓ Сообщение отправлено")
    except Exception as e:
        print(f"✗ Ошибка отправки: {e}")

if __name__ == "__main__":
    print("=" * 60)
    print("ТЕСТ СЕРВЕРА ТЕЛЕМЕТРИИ")
    print("=" * 60)
    
    send_test_messages()
    
 
    time.sleep(1)
    
    
    check_http_api()
    
    print("\n" + "=" * 60)
    print("ДОПОЛНИТЕЛЬНЫЙ ТЕСТ")
    print("=" * 60)
    
   
    debug_server()
    
    time.sleep(1)
    print("\nФинальная проверка устройства 1:")
    result = subprocess.run(
        ['curl', '-s', 'http://localhost:8080/device/1/latest'],
        capture_output=True, text=True
    )
    print(result.stdout)