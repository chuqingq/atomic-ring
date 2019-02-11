import ctypes
import time

for count in range(10):
    # 创建动态数组
    size = 4
    int_arr4 = ctypes.c_int*size
    int_arr = int_arr4()
    for s in range(size):
        int_arr[s] = count + s
    print(int_arr)
    # 传入
    mylib = ctypes.cdll.LoadLibrary("producer_consumer.so")
    mylib.init()
    mylib.produce(int_arr, size)
    # sleep
    time.sleep(1)
