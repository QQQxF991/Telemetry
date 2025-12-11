#!/usr/bin/env python3
"""
Базовый тест сервера телеметрии
"""
import socket
import struct
import time
import subprocess
import json

def create_message(device_id, value, timestamp):
    device_bytes = bytes([device_id])
    value_bytes = struct.pack('>f', value)  
    timestamp_bytes = struct.pack('>Q', timestamp)  
    
    data = device_bytes + value_bytes + timestamp_bytes
    
    crc = 0
    for byte in data:
        crc ^= byte
    
    return data + bytes([crc])

def send_message(device_id, value, timestamp=None):
    if timestamp is None:
        timestamp = int(time.time())
    
    message = create_message(device_id, value, timestamp)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 9001))
        sock.send(message)
        sock.close()
        return True
    except Exception as e:
        print(f"Ошибка отправки: {e}")
        return False

def test_single_device():
    print("Тест одного устройства...")
    
    for i in range(5):
        value = 10.0 + i * 5.0
        timestamp = int(time.time()) - (4 - i) * 10 
        if send_message(1, value, timestamp):
            print(f"  Отправлено: device=1, value={value}, timestamp={timestamp}")
        time.sleep(0.1)
    
    print("✓ Сообщения отправлены")

def test_multiple_devices():
    print("\nТест нескольких устройств...")
    
    devices = [1, 2, 3, 10, 20]
    for device_id in devices:
        value = device_id * 10.0
        if send_message(device_id, value):
            print(f"  Отправлено: device={device_id}, value={value}")
        time.sleep(0.1)
    
    print("✓ Сообщения для всех устройств отправлены")

def check_http_endpoint(device_id, endpoint):
    try:
        cmd = ['curl', '-s', f'http://localhost:8080/device/{device_id}/{endpoint}']
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=3)
        
        if result.returncode == 0:
            try:
                return json.loads(result.stdout)
            except:
                return result.stdout
        else:
            return None
    except Exception as e:
        print(f"Ошибка проверки {endpoint}: {e}")
        return None

def test_latest_endpoint():
    print("\nТестирование /latest эндпоинтов:")
    
    for device_id in [1, 2, 3, 99]:  
        result = check_http_endpoint(device_id, "latest")
        print(f"  device {device_id}: {result}")

def test_stats_endpoint():
    print("\nТестирование /stats эндпоинтов:")
    
    for device_id in [1, 2, 3, 99]:
        result = check_http_endpoint(device_id, "stats")
        print(f"  device {device_id}: {result}")

def test_crc_error():
    print("\nТест с некорректным CRC...")
    
    device_id = 1
    value = 100.0
    timestamp = int(time.time())
    
    message = create_message(device_id, value, timestamp)
    
    wrong_message = message[:-1] + bytes([message[-1] ^ 0xFF])
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 9001))
        sock.send(wrong_message)
        sock.close()
        print("✓ Сообщение с неправильным CRC отправлено (должно быть отклонено сервером)")
    except Exception as e:
        print(f"Ошибка: {e}")

def test_performance():
    print("\nТест производительности (20 сообщений)...")
    
    start_time = time.time()
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(('localhost', 9001))
        
        for i in range(20):
            device_id = (i % 5) + 1
            value = 50.0 + i * 2.0
            timestamp = int(time.time()) - i
            
            message = create_message(device_id, value, timestamp)
            sock.send(message)
            
            if (i + 1) % 5 == 0:
                print(f"  Отправлено {i + 1} сообщений")
            
            time.sleep(0.01)
        
        sock.close()
        
        elapsed = time.time() - start_time
        print(f"✓ Производительность: 20 сообщений за {elapsed:.2f} сек ({20/elapsed:.1f} сообщ/сек)")
        
    except Exception as e:
        print(f"Ошибка производительности: {e}")

if __name__ == "__main__":
    print("=" * 60)
    print("БАЗОВЫЙ ТЕСТ СЕРВЕРА ТЕЛЕМЕТРИИ")
    print("=" * 60)
    
    print("Ожидание запуска сервера...")
    time.sleep(2)
    
    
    test_single_device()
    
    
    test_multiple_devices()
    
    
    print("\nОжидание обработки сообщений...")
    time.sleep(1)
    
    
    test_latest_endpoint()
    test_stats_endpoint()
    test_crc_error()
    test_performance()
    
    print("\n" + "=" * 60)
    print("ТЕСТ ЗАВЕРШЕН")
    print("=" * 60)