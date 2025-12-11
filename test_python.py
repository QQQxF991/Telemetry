#!/usr/bin/env python3
import socket
import struct
import time

def create_correct_binary_message(device_id, value, timestamp):
    device_id_bytes = bytes([device_id])
    value_bytes = struct.pack('>f', value)  
    timestamp_bytes = struct.pack('>Q', timestamp)  
    
    
    data = device_id_bytes + value_bytes + timestamp_bytes
    
    crc = 0
    for byte in data:
        crc ^= byte
    
   
    return data + bytes([crc])

def test_correct_message():
    print("Отправка корректного бинарного сообщения...")
    
   
    device_id = 1
    value = 42.5
    timestamp = int(time.time())
    
    
    message = create_correct_binary_message(device_id, value, timestamp)
    
    print(f"Сообщение ({len(message)} байт):")
    print("  device_id:", device_id)
    print("  value:", value)
    print("  timestamp:", timestamp)
    print("  CRC:", message[-1])
    
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 9001))
        sock.send(message)
        
        
        time.sleep(0.5)
        sock.close()
        
        print("✓ Сообщение отправлено")
        
        
        print("\nПроверка HTTP API через 2 секунды...")
        time.sleep(2)
        
        
        import subprocess
        print("GET /device/1/latest:")
        result = subprocess.run(['curl', '-s', 'http://localhost:8080/device/1/latest'], 
                              capture_output=True, text=True)
        print(result.stdout)
        
        print("\nGET /device/1/stats:")
        result = subprocess.run(['curl', '-s', 'http://localhost:8080/device/1/stats'], 
                              capture_output=True, text=True)
        print(result.stdout)
        
    except Exception as e:
        print(f"✗ Ошибка: {e}")

def test_multiple_messages():
    
    print("\nОтправка 10 сообщений...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 9001))
        
        for i in range(10):
            device_id = i % 3 + 1  
            value = 10.0 + i * 2.5
            timestamp = int(time.time()) - i * 10
            
            message = create_correct_binary_message(device_id, value, timestamp)
            sock.send(message)
            print(f"  Отправлено: устройство={device_id}, значение={value}")
            time.sleep(0.1)
        
        sock.close()
        print("✓ Сообщения отправлены")
        
    except Exception as e:
        print(f"✗ Ошибка: {e}")

if __name__ == "__main__":
    print("=" * 60)
    print("ИСПРАВЛЕННЫЙ ТЕСТ СЕРВЕРА ТЕЛЕМЕТРИИ")
    print("=" * 60)
    
    test_correct_message()
    
    test_multiple_messages()
    
    print("\n" + "=" * 60)
    print("ИТОГОВАЯ ПРОВЕРКА:")
    print("=" * 60)
    
    
    import subprocess
    
    for device_id in [1, 2, 3]:
        print(f"\n--- Устройство {device_id} ---")
        
        
        print("GET /latest:")
        result = subprocess.run(['curl', '-s', f'http://localhost:8080/device/{device_id}/latest'], 
                              capture_output=True, text=True)
        print(f"  {result.stdout.strip()}")
        
        
        print("GET /stats:")
        result = subprocess.run(['curl', '-s', f'http://localhost:8080/device/{device_id}/stats'], 
                              capture_output=True, text=True)
        print(f"  {result.stdout.strip()}")
    
    print("\nТест завершен!")