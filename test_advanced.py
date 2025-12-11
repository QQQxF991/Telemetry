#!/usr/bin/env python3

import socket
import struct
import time
import random
import statistics
import json
import urllib.request
import urllib.error

class TelemetryTester:
    def __init__(self, host='localhost', binary_port=9001, http_port=8080):
        self.host = host
        self.binary_port = binary_port
        self.http_port = http_port
        
    def create_message(self, device_id, value, timestamp):
        device_bytes = bytes([device_id])
        value_bytes = struct.pack('>f', value)
        timestamp_bytes = struct.pack('>Q', timestamp)
        
        data = device_bytes + value_bytes + timestamp_bytes
        
        crc = 0
        for byte in data:
            crc ^= byte
        
        return data + bytes([crc])
    
    def send_messages_batch(self, messages):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10)
            sock.connect((self.host, self.binary_port))
            
            for device_id, value, timestamp in messages:
                message = self.create_message(device_id, value, timestamp)
                sock.send(message)
            
            sock.close()
            return True
        except Exception as e:
            print(f"Ошибка отправки пакета: {e}")
            return False
    
    def query_api(self, endpoint):
        try:
            url = f"http://{self.host}:{self.http_port}{endpoint}"
            req = urllib.request.Request(url, method='GET')
            req.add_header('User-Agent', 'TelemetryTester/1.0')
            
            with urllib.request.urlopen(req, timeout=5) as response:
                data = response.read().decode('utf-8')
                return {
                    'status': response.status,
                    'data': json.loads(data) if data else None
                }
        except urllib.error.HTTPError as e:
            return {'status': e.code, 'data': json.loads(e.read().decode('utf-8'))}
        except Exception as e:
            return {'status': 0, 'error': str(e)}
    
    def test_ring_buffer(self, device_id=1):
        print(f"\nТест кольцевого буфера для устройства {device_id}...")
        
       
        messages = []
        base_time = int(time.time())
        
        for i in range(60):
            value = random.uniform(-100.0, 100.0)
            timestamp = base_time - (59 - i)  
            messages.append((device_id, value, timestamp))
        
        if self.send_messages_batch(messages):
            print(f"  Отправлено 60 сообщений")
            
           
            time.sleep(2)
            
            
            result = self.query_api(f"/device/{device_id}/stats")
            if result['status'] == 200 and 'data' in result:
                data = result['data']
                print(f"  Статистика: count={data.get('count', 0)}, min={data.get('min', 0):.2f}, "
                      f"max={data.get('max', 0):.2f}, avg={data.get('average', 0):.2f}")
                
                if data.get('count', 0) <= 50:
                    print("✓ Кольцевой буфер работает корректно")
                else:
                    print("✗ Кольцевой буфер превысил размер 50")
            else:
                print(f"✗ Ошибка получения статистики: {result}")
    
    def test_device_limits(self):
        print("\nТест граничных значений device_id...")
        
        test_cases = [
            (0, 1.0, "минимальный device_id (0)"),
            (255, 255.0, "максимальный device_id (255)"),
            (128, 128.0, "средний device_id (128)"),
        ]
        
        for device_id, value, description in test_cases:
            message = self.create_message(device_id, value, int(time.time()))
            
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((self.host, self.binary_port))
                sock.send(message)
                sock.close()
                print(f"✓ Отправлено: {description}")
            except Exception as e:
                print(f"✗ Ошибка для {description}: {e}")
            
            time.sleep(0.1)
    
    def test_value_ranges(self):
        print("\nТест различных диапазонов значений...")
        
        test_values = [
            (0.0, "ноль"),
            (-0.0, "отрицательный ноль"),
            (3.14159265, "PI"),
            (-273.15, "абсолютный ноль"),
            (1.0e-10, "очень маленькое значение"),
            (1.0e10, "очень большое значение"),
            (float('inf'), "бесконечность"),
        ]
        
        device_id = 100
        
        for value, description in test_values:
            if not isinstance(value, float):
                continue
                
            message = self.create_message(device_id, value, int(time.time()))
            
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((self.host, self.binary_port))
                sock.send(message)
                sock.close()
                print(f"✓ Отправлено: {description} = {value}")
            except Exception as e:
                print(f"✗ Ошибка для {description}: {e}")
            
            time.sleep(0.1)
    
    def test_concurrent_devices(self):
        print("\nТест 50 разных устройств...")
        
        success_count = 0
        for device_id in range(1, 51):
            value = device_id * 1.5
            message = self.create_message(device_id, value, int(time.time()))
            
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(2)
                sock.connect((self.host, self.binary_port))
                sock.send(message)
                sock.close()
                success_count += 1
            except:
                pass
            
            time.sleep(0.01)
        
        print(f"  Успешно отправлено: {success_count}/50 устройств")
        
        time.sleep(1)
        for device_id in random.sample(range(1, 51), 5):
            result = self.query_api(f"/device/{device_id}/latest")
            if result['status'] == 200:
                print(f"  Устройство {device_id}: данные доступны")
            else:
                print(f"  Устройство {device_id}: данные недоступны")
    
    def test_error_handling(self):
        print("\nТест обработки ошибок...")
        
        result = self.query_api("/invalid/path")
        print(f"  Несуществующий endpoint: статус {result.get('status', 'N/A')}")
        
        result = self.query_api("/device/999999/latest")
        print(f"  Несуществующее устройство: статус {result.get('status', 'N/A')}")
        
        result = self.query_api("/device/invalid/latest")
        print(f"  Некорректный device_id: статус {result.get('status', 'N/A')}")
        
        try:
            url = f"http://{self.host}:{self.http_port}/device/1/latest"
            req = urllib.request.Request(url, method='POST', data=b'test')
            with urllib.request.urlopen(req, timeout=2) as response:
                print(f"  POST метод: статус {response.status}")
        except urllib.error.HTTPError as e:
            print(f"  POST метод: ожидаемый статус {e.code}")

def main():
    print("=" * 70)
    print("РАСШИРЕННЫЙ ТЕСТ СЕРВЕРА ТЕЛЕМЕТРИИ")
    print("=" * 70)
    
    tester = TelemetryTester()
    
    print("Ожидание запуска сервера...")
    time.sleep(3)
    
    
    tests = [
        ("Кольцевой буфер", tester.test_ring_buffer),
        ("Граничные значения device_id", tester.test_device_limits),
        ("Различные диапазоны значений", tester.test_value_ranges),
        ("Множество устройств", tester.test_concurrent_devices),
        ("Обработка ошибок", tester.test_error_handling),
    ]
    
    for test_name, test_func in tests:
        print(f"\n{'='*40}")
        print(f"ТЕСТ: {test_name}")
        print('='*40)
        try:
            test_func()
        except Exception as e:
            print(f"Ошибка во время теста {test_name}: {e}")
    
    print("\n" + "=" * 70)
    print("ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ")
    print("=" * 70)

if __name__ == "__main__":
    main()