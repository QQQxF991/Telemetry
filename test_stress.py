#!/usr/bin/env python3
import socket
import struct
import time
import random
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed

def create_message(device_id, value, timestamp):
    
    device_bytes = bytes([device_id])
    value_bytes = struct.pack('>f', value)
    timestamp_bytes = struct.pack('>Q', timestamp)
    
    data = device_bytes + value_bytes + timestamp_bytes
    
    crc = 0
    for byte in data:
        crc ^= byte
    
    return data + bytes([crc])

def send_messages_batch(num_messages, thread_id):
   
    sent = 0
    start_time = time.time()
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(30)
        sock.connect(('localhost', 9001))
        
        for i in range(num_messages):
            device_id = random.randint(1, 20)
            value = random.uniform(-100.0, 100.0)
            timestamp = int(time.time()) - random.randint(0, 1000)
            
            message = create_message(device_id, value, timestamp)
            sock.send(message)
            sent += 1
            
            if i % 100 == 0:
                time.sleep(0.001)
        
        sock.close()
        
    except Exception as e:
        print(f"Поток {thread_id}: ошибка - {e}")
    
    elapsed = time.time() - start_time
    return thread_id, sent, elapsed

def stress_test():
    
    print("=" * 60)
    print("НАГРУЗОЧНЫЙ ТЕСТ СЕРВЕРА ТЕЛЕМЕТРИИ")
    print("=" * 60)
    
    print("Конфигурация:")
    print("  Потоки: 5")
    print("  Сообщений на поток: 200")
    print("  Всего сообщений: 1000")
    print()
    
    start_total = time.time()
    
    with ThreadPoolExecutor(max_workers=5) as executor:
        futures = []
        for i in range(5):
            future = executor.submit(send_messages_batch, 200, i+1)
            futures.append(future)
        
        total_sent = 0
        total_time = 0
        
        for future in as_completed(futures):
            thread_id, sent, elapsed = future.result()
            print(f"Поток {thread_id}: отправлено {sent} сообщений за {elapsed:.2f} сек "
                  f"({sent/elapsed:.1f} сообщ/сек)")
            total_sent += sent
            total_time = max(total_time, elapsed)
    
    total_elapsed = time.time() - start_total
    
    print("\n" + "=" * 40)
    print("ИТОГИ НАГРУЗОЧНОГО ТЕСТА:")
    print("=" * 40)
    print(f"Всего отправлено: {total_sent} сообщений")
    print(f"Общее время: {total_elapsed:.2f} секунд")
    print(f"Средняя скорость: {total_sent/total_elapsed:.1f} сообщений/сек")
    
    if total_sent >= 900:
        print("✓ Тест пройден: сервер выдержал нагрузку")
    else:
        print("✗ Тест не пройден: слишком много ошибок")

def continuous_load(duration=60):
    print(f"\nНепрерывная нагрузка в течение {duration} секунд...")
    print("Нажмите Ctrl+C для досрочной остановки")
    
    sent = 0
    start_time = time.time()
    end_time = start_time + duration
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(duration + 5)
        sock.connect(('localhost', 9001))
        
        while time.time() < end_time:
            device_id = random.randint(1, 50)
            value = random.uniform(-500.0, 500.0)
            timestamp = int(time.time())
            
            message = create_message(device_id, value, timestamp)
            sock.send(message)
            sent += 1
            
            if int(time.time() - start_time) % 5 == 0 and int(time.time()) != int(start_time):
                elapsed = time.time() - start_time
                print(f"  Отправлено {sent} сообщений ({sent/elapsed:.1f} сообщ/сек)")
                time.sleep(0.1) 
        
        sock.close()
        
    except KeyboardInterrupt:
        print("\nТест прерван пользователем")
    except Exception as e:
        print(f"Ошибка: {e}")
    
    elapsed = time.time() - start_time
    print(f"\nРезультат непрерывной нагрузки:")
    print(f"  Отправлено: {sent} сообщений")
    print(f"  Время: {elapsed:.2f} секунд")
    print(f"  Скорость: {sent/elapsed:.1f} сообщений/сек")

if __name__ == "__main__":
    print("Ожидание запуска сервера (5 секунд)...")
    time.sleep(5)
    
   
    stress_test()
    
    print("\n" + "=" * 60)
    print("НАГРУЗОЧНЫЙ ТЕСТ ЗАВЕРШЕН")
    print("=" * 60)